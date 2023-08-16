#include "../include/head/NoCopyable.h"
#include "FdGuard.hpp"
#include "EpollPoller.h"
#include "TimerContainer.h"
#include "console_log.hpp"

// thread_local 变量提供了一种在线程间共享数据时避免竞态条件的方式。每个线程操作自己的副本，不需要使用锁或其他同步机制。
// 需要在每个线程中初始化 tes 变量，以确保每个线程都有自己的 Reactor 实例。
static thread_local Reactor* tes;

// 反应堆线程,Reactor包装了EpollPoller，并提供了任务队列，以使其它拥有Reactor对象指针的线程可以向IO线程添加任务。
class Reactor :NoCopyable
{
private:
	// 在不同线程中调用 epoll_create1(EPOLL_CLOEXEC) 将创建多个独立的 epoll 实例，它们之间不会共享任何状态。
	// 多个线程或进程同时监听不同的事件，以实现高并发的网络处理。
	EpollPoller poller;

public:
	EpollPoller& GetPoller() { return poller; }

private:
	const thread::id tid{ std::this_thread::get_id() }; // 在IO ThreadPOOL中 创建线程时 ，同时创建Reactor,并获得当时创建的线程id

public:
	[[nodiscard]] thread::id GetTid() const { return tid; }
	[[nodiscard]] bool InIOThread() const { return tid == std::this_thread::get_id(); }

private:
	bool running{ true };

public:
	void React(); //只能在创建Reactor对象的线程中调用，不能跨线程调用

	void quit(); // todo: 使用shared_ptr<Reactor> 调用quit(),以确保线程安全。

private:
	mutable mutex mtx; //被声明为 mutable，表示即使在一个被声明为 const 的成员函数内，该成员变量仍然可以被修改。
	vector<function<void()>> tasks;  // 任务队列
	bool handling_tasks{ false };
	void HandleTask();  // 创建临时任务队列，并执行

public:
	void AddTask(function<void()> cb); // 加锁后向tasks中添加任务
	void Execute(function<void()> cb); // 被HandleEvent()或HandleTask()的中执行的任务函数调用。如果在io线程中,直接执行回调函数;否则执行AddTask()。

private:
	TimerContainer timer_container; // 定时器，外部事件可以向Reactor注册定时事件
public:
	int i = 0;
	void CallAt(const TimeStamp time, const function<void()>& cb, Timer*& timer) { Execute([this, time, cb, &timer]() {timer_container.AddTimer(cb, time, 0.0, timer); }); } // 不能对这里的cb使用move, 否则会使得cb为空。
	void CallAfter(const double delay, const function<void()>& cb, Timer*& timer) { Execute([this, delay, cb, &timer]() {timer_container.AddTimer(cb, AddSeconds(TimeStamp::Now(), delay), 0.0, timer); }); } // 经过delay秒后调用
	void CallEvery(const double interval, const function<void()>& cb, Timer*& timer) { Execute([this, interval, cb, &timer]() {timer_container.AddTimer(cb, AddSeconds(TimeStamp::Now(), interval), interval, timer); }); } // 每隔interval秒调用(第一次调用是在当前时间经过interval秒后)
	void CancelCall(Timer* p) { AddTask([this, p]() {timer_container.CancelTimer(p); }); }
public:
	Reactor() : timer_container(poller)
	{
		if (tes != nullptr) { err("tid为 ", tid, " 的线程中创建了多个EventScheduler对象！") }
		tes = this; log("tid为 ", tid, " 的线程创建了EventScheduler对象，该线程成为IO线程。")
	}
	~Reactor()
	{
		tes = nullptr; log("tid为 ", tid, " 的线程中的EventScheduler对象销毁，该IO线程将要退出。")
	}


};