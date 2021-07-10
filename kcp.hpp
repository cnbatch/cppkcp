//=====================================================================
//
// KCP - A Better ARQ Protocol Implementation
// Original author: skywind3000 (at) gmail.com, 2010-2011
// Modifier: cnbatch, 2021
//  
// Features:
// + Average RTT reduce 30% - 40% vs traditional ARQ like tcp.
// + Maximum RTT reduce three times vs tcp.
// + Lightweight, distributed as a single source file.
//
//=====================================================================
#ifndef __KCP_HPP__
#define __KCP_HPP__

#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <functional>
#include <list>
#include <vector>


//=====================================================================
// QUEUE DEFINITION                                                  
//=====================================================================
#ifndef __IQUEUE_DEF__
#define __IQUEUE_DEF__


#ifdef _MSC_VER
#pragma warning(disable:4311)
#pragma warning(disable:4312)
#pragma warning(disable:4996)
#endif

#endif


namespace kcp
{
	//---------------------------------------------------------------------
	// BYTE ORDER & ALIGNMENT
	//---------------------------------------------------------------------
#ifndef IWORDS_BIG_ENDIAN
#ifdef _BIG_ENDIAN_
#if _BIG_ENDIAN_
#define IWORDS_BIG_ENDIAN 1
#endif
#endif
#ifndef IWORDS_BIG_ENDIAN
#if defined(__hppa__) || \
            defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
            (defined(__MIPS__) && defined(__MIPSEB__)) || \
            defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
            defined(__sparc__) || defined(__powerpc__) || \
            defined(__mc68000__) || defined(__s390x__) || defined(__s390__)
#define IWORDS_BIG_ENDIAN 1
#endif
#endif
#ifndef IWORDS_BIG_ENDIAN
#define IWORDS_BIG_ENDIAN  0
#endif
#endif

#ifndef IWORDS_MUST_ALIGN
#if defined(__i386__) || defined(__i386) || defined(_i386_)
#define IWORDS_MUST_ALIGN 0
#elif defined(_M_IX86) || defined(_X86_) || defined(__x86_64__)
#define IWORDS_MUST_ALIGN 0
#elif defined(__amd64) || defined(__amd64__)
#define IWORDS_MUST_ALIGN 0
#else
#define IWORDS_MUST_ALIGN 1
#endif
#endif

	constexpr int KCP_LOG_OUTPUT    =    1;
	constexpr int KCP_LOG_INPUT     =    2;
	constexpr int KCP_LOG_SEND      =    4;
	constexpr int KCP_LOG_RECV      =    8;
	constexpr int KCP_LOG_IN_DATA   =   16;
	constexpr int KCP_LOG_IN_ACK    =   32;
	constexpr int KCP_LOG_IN_PROBE  =   64;
	constexpr int KCP_LOG_IN_WINS   =  128;
	constexpr int KCP_LOG_OUT_DATA  =  256;
	constexpr int KCP_LOG_OUT_ACK   =  512;
	constexpr int KCP_LOG_OUT_PROBE = 1024;
	constexpr int KCP_LOG_OUT_WINS  = 2048;

	namespace internal_impl
	{
		//=====================================================================
		// SEGMENT
		//=====================================================================
		struct segment
		{
			uint32_t conv = 0;
			uint32_t cmd = 0;
			uint32_t frg = 0;
			uint32_t wnd = 0;
			uint32_t ts = 0;
			uint32_t sn = 0;
			uint32_t una = 0;
			uint32_t resendts = 0;
			uint32_t rto = 0;
			uint32_t fastack = 0;
			uint32_t xmit = 0;
			std::vector<char> data;

			segment() = default;
			segment(size_t sizes)
			{
				data.resize(sizes);
			}
		};
	}

	//---------------------------------------------------------------------
	// KCP
	//---------------------------------------------------------------------
	class kcp
	{
	private:
		uint32_t conv, mtu, mss, state;
		uint32_t snd_una, snd_nxt, rcv_nxt;
		uint32_t ts_recent, ts_lastack, ssthresh;
		int32_t rx_rttval, rx_srtt, rx_rto, rx_minrto;
		uint32_t snd_wnd, rcv_wnd, rmt_wnd, cwnd, probe;
		uint32_t current, interval, ts_flush, xmit;
		uint32_t nodelay, updated;
		uint32_t ts_probe, probe_wait;
		uint32_t dead_link, incr;
		std::list<internal_impl::segment> snd_queue;
		std::list<internal_impl::segment> rcv_queue;
		std::list<internal_impl::segment> snd_buf;
		std::list<internal_impl::segment> rcv_buf;
		std::vector<std::pair<uint32_t, uint32_t>> acklist;
		uint32_t ackblock;
		void *user;
		std::vector<char> buffer;
		int fastresend;
		int fastlimit;
		int nocwnd, stream;
		int logmask;
		std::function<int(const char *, int, void *)> output;	// int(*output)(const char *buf, int len, void *user)
		std::function<void(const char *, void *)> writelog;	//void(*writelog)(const char *log, void *user)

		static char * encode8u(char *p, unsigned char c);
		static const char * decode8u(const char *p, unsigned char *c);
		static char * encode16u(char *p, unsigned short w);
		static const char * decode16u(const char *p, unsigned short *w);
		static char * encode32u(char *p, uint32_t l);
		static const char * decode32u(const char *p, uint32_t *l);
		static char * encode_seg(char *ptr, const internal_impl::segment &seg);
		void print_queue(const char *name, const std::list<internal_impl::segment> &segment);

	public:
		//---------------------------------------------------------------------
		// interface
		//---------------------------------------------------------------------

		// create a new kcp control object, 'conv' must equal in two endpoint
		// from the same connection. 'user' will be passed to the output callback
		// output callback can be setup like this: 'kcp->output = my_udp_output'
		kcp(uint32_t conv, void *user);

		// release kcp control object
		~kcp();

		// set output callback, which will be invoked by kcp
		// int(*output)(const char *buf, int len, void *user)
		void set_output(std::function<int(const char *, int, void *)> output);

		// user/upper level recv: returns size, returns below zero for EAGAIN
		int receive(char *buffer, int len);
		int receive(std::vector<char> &buffer);

		// user/upper level send, returns below zero for error
		int send(const char *buffer, int len);

		// update state (call it repeatedly, every 10ms-100ms), or you can ask 
		// check when to call it again (without Input/_send calling).
		// 'current' - current timestamp in millisec. 
		void update(uint32_t current);

		// Determine when should you invoke Update:
		// returns when you should invoke Update in millisec, if there 
		// is no Input/_send calling. you can call Update in that
		// time, instead of call update repeatly.
		// Important to reduce unnacessary Update invoking. use it to 
		// schedule Update (eg. implementing an epoll-like mechanism, 
		// or optimize Update when handling massive kcp connections)
		uint32_t check(uint32_t current);

		// when you received a low level packet (eg. UDP packet), call it
		int input(const char *data, long size);

		// flush pending data
		void flush();

		// check the size of next message in the recv queue
		int PeekSize();

		// change MTU size, default is 1400
		int set_mtu(int mtu);
		int get_mtu();

		// set maximum window size: sndwnd=32, rcvwnd=32 by default
		void set_window_size(int sndwnd, int rcvwnd);
		void get_window_size(int &sndwnd, int &rcvwnd);

		// get how many packet is waiting to be sent
		int waiting_for_send();

		// fastest: no_delay(1, 20, 2, 1)
		// nodelay: 0:disable(default), 1:enable
		// interval: internal update timer interval in millisec, default is 100ms 
		// resend: 0:disable fast resend(default), 1:enable fast resend
		// nc: 0:normal congestion control(default), 1:disable congestion control
		int no_delay(int nodelay, int interval, int resend, int nc);


		void write_log(int mask, const char *fmt, ...);

		// read conv
		static uint32_t get_conv(const void *ptr);
		uint32_t get_conv();

		// check log mask
		bool can_log(int mask);

		int get_interval(int interval);

		void set_stream_mode(bool enable);

		int32_t& rx_min_rto();
		int& log_mask();

	protected:

		int kcp_output(const void *data, int size);
		void update_ack(int32_t rtt);
		void shrink_buffer();
		void parse_ack(uint32_t sn);
		void parse_una(uint32_t una);
		void parse_fast_ack(uint32_t sn, uint32_t ts);
		void parse_data(internal_impl::segment &newseg);
		int window_unused();
	};
}


#endif


