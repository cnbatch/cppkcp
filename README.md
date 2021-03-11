KCP in C++ - Simply re-write iKCP to support STL containers.
======================================

Original Author of KCP: [skywind3000](https://github.com/skywind3000/kcp) 

# 简介

以 C++ 的方式修改了 KCP 源码，用 STL 容器替代原作的手工链表。

现在 `ikcp_recv` (新名字：`KCP::Receive`) 可以接受空 `std::vector` 并自动扩充 `vector` 的容量。

# Overview

Modified KCP source code in C++ style. Original's manual Linked list is replaced with STL container.

`ikcp_recv` (New name: `KCP::Receive`) can now accept an empty `std::vector`. The size of this `vector` will be extended automatically.