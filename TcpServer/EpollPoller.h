#pragma once
#include "FdGuard.hpp"
#include "../include/head/NoCopyable.h"
#include "../include/head/cpp_container.h"
#include "EventRegister.h"
#include "File_Operation.h"  //统一管理文件标识符
#include "console_log.hpp"


class EpollPoller : NoCopyable
{
private:
	FdGuard epoll_fd;                              // 定义epoll句柄
	vector<struct epoll_event> events_vector;      // 存储返回epoll_event的容器，可自动增加 
	vector<EventRegister*> active_registers;       // 将epoll中的事件与eventregister绑定，存储出发时间的容器中

public:
	void poll(int ms_timeout = 10'000); // 超时时间默认为10秒，从epollfd 句柄中获取已经触发的事务
	void HandleActiveEvents();          // 处理触发事件

private:
	void epoll_control(int operation, EventRegister* er) const;   // 操作epoll，比如加入事件
	static const int first_register = -1; // EventRegister为首次注册
	static const int registered = 1;      // EventRegister已经注册
	static const int unregistered = 2;    // EventRegister尚未注册

public:
	void Register(EventRegister* er) const; // 将事件注册到Epoll中观测
	void Unregister(EventRegister* er) const; // 将事件从epoll移除

	// 唤醒
private:
	FdGuard wakeup_fd;
	EventRegister wakeup_register;

public:
	void WakeUp() const
	{
		unsigned long long one = 1;
		ssize_t ret = write(wakeup_fd.fd, &one, sizeof one);
		if (ret != sizeof one) { perror("EpollPoller::WakeUp::write"); }
	}

public:
	explicit EpollPoller();
	~EpollPoller() = default;

};