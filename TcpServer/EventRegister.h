#pragma once
#include <sys/epoll.h>
#include <cpp_header/cpp_smartptr.h>
#include "NoCopyable.h"

//在不包含完整定义的情况下引入实体的名称，用于解决头文件的循环包含问题和提高编译效率。
class EpollPoller;

// 事件操作类的封装
class EventRegister : NoCopyable
{
private:
	EpollPoller& poller;

	const int fd;              // fd
	int events{ 0 };           // fd上关注的事件
	int triggered_events{ 0 }; // fd上到来的事件
	int state{ -1 };           // 注册状态，是否已经注册到EpollPoller

public:
	[[nodiscard]] int Fd() const { return fd; }                     // 返回文件标识符
	[[nodiscard]] int Events() const { return events; }             // 返回事件
	[[nodiscard]] int TriggeredEvents() const { return triggered_events; }  // 返回fd上到来的事件
	void SetTriggeredEvents(int t) { triggered_events = t; }        //   设置到来的事件,如EPOLLIIN	等掩码
	[[nodiscard]] int State() const { return state; }               //   查看是否已经注册到EpollPoller
	void SetState(int i) { state = i; }                             //   设置 注册状态，分为首次，已经，尚未注册

	void Remove();                                                  // 把事件移除Epoll
	void Update();                                                  // 把事件注册到Epoll中

	static const int none_event = 0;                                // 定义空事件
	static const int read_event = EPOLLIN | EPOLLPRI;               // 定义读事件 
	static const int write_event = EPOLLOUT;                        // 定义写事件

	void InterestReadableEvent() { events |= read_event; Update(); } // 关注fd的可读事件 执行 a = a | b
	void InterestWritableEvent() { events |= write_event; Update(); } // 关注fd的可写事件
	void UnInterestReadableEvent() { events &= ~read_event; Update(); } // 取消关注fd的可读事件 执行 events = events & ~read_event
	void UnInterestWritableEvent() { events &= ~write_event; Update(); } // 取消关注fd的可读事件
	void UnInterestAllEvents() { if (events == none_event) { return; } events = none_event; Update(); } // 取消关注fd的所有事件

	[[nodiscard]] bool MonitoringReadable() const { return events & read_event; }   // 判断事件是不是读事件
	[[nodiscard]] bool MonitoringWritable() const { return events & write_event; }  // 判断事件是不是写事件
	[[nodiscard]] bool MonitoringNothing() const { return events == none_event; }   // 判断事件是不是空时间

private:
	function<void()> connection_callback;
	function<void()> read_callback;
	function<void()> write_callback;
	function<void()> error_callback;

public:
	void SetCloseCallBack(function<void()> cb) { connection_callback = move(cb); }  // 设置连接回调函数
	void SetReadCallBack(function<void()> cb) { read_callback = move(cb); }         // 设置读取回调函数
	void SetWriteCallBack(function<void()> cb) { write_callback = move(cb); }       // 设置写入回调函数
	void SetErrorCallBack(function<void()> cb) { error_callback = move(cb); }       // 设置设置错误回调函数

private:
	weak_ptr<void> hold_ptr; // 与TcpConnection对象的生存期有关，weak_ptr不控制对象的生命周期，但是，它知道对象是否还活着。查看所指向的对象是否有效
	bool held{ false };      // 是否持有TcpConnention的资源
public:
	void hold(const shared_ptr<void>& obj) { hold_ptr = obj; held = true; } // 去持有TcpConnection的资源

public:
	void HandleEvent();        // 处理事件
	// EventRegister::HandleEvent() 函数根据 triggered_events 中的事件标志，执行相应的事件回调函数，用于处理不同类型的事件。
	// 同时，它还处理可能存在的 hold_ptr，以确保资源的生命周期控制

public:
	EventRegister(EpollPoller& epoller, int f) : poller(epoller), fd(f) { }
	~EventRegister() { UnInterestAllEvents(); Remove(); }
};