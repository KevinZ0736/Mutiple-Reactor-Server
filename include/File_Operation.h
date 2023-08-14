#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include "TimeStamp.hpp"

//以下这些函数执行失败时，会直接退出程序
#pragma once
extern int open_socket(); // 创建SOCK_NONBLOCK | SOCK_CLOEXEC的tcp socket, 并设置端口复用。
extern int open_epoll(); // 创建EPOLL_CLOEXEC的epoll fd
extern int open_null(); // 以O_RDONLY | O_CLOEXEC 打开null文件
extern int open_eventfd(); // eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
extern int open_timerfd();
extern void read_timerfd(int timerfd);
extern void reset_timerfd(int timerfd, long expiration);