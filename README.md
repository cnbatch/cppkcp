KCP in C++ : Thread-safe iKCP
======================================

Original Author of KCP: [skywind3000](https://github.com/skywind3000/kcp) 

# 简介

以 C++ 的方式修改了 KCP 源码，用 STL 容器替代原作的手工链表，用以改善性能。

外包装了一层，**线程安全**，可以同时收发。

# Overview

Modified KCP source code in C++ style. Original's manual Linked list is replaced with STL container, for performance improvement.

Wrapped a layer, **Thread-safe**, KCP can now send and receive data simultaneously.