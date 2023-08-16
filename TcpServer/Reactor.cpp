#include "Reactor.h"

void Reactor::React()
{
	if (!InIOThread()) { fatal("tid为 ", tid, " 的线程启动了其它IO线程的事件循环！") }
	while (running)
	{
		poller.HandleActiveEvents();
		HandleTask();
	}
}


//通常情况下，Reactor 对象的事件循环会在 IO 线程中运行，它会不断地监听和处理事件。
//当在非 IO 线程中调用 quit() 函数时，由于当前线程不是 IO 线程，直接设置 running 为 false 可能无法立即停止事件循环，
//因此使用 poller.WakeUp() 来唤醒事件循环，使其退出阻塞状态并检查 running 标志。
//这样设计可以确保即使在非 IO 线程中调用 quit() 函数，也能够尽快退出 IO 线程的事件循环，以达到停止 Reactor 对象的目的。

// while(running){~ listen() ~} 通过写入唤醒listen()
void Reactor::quit() // 使用shared_ptr<Reactor> 调用quit(),以确保线程安全。
{
	running = false;
	if (!InIOThread()) { poller.WakeUp(); } //log("tid为 ", tid, " 的线程中的EventScheduler对象将要结束事件循环。")
}

void Reactor::HandleTask()
{
	vector<function<void()>> temp; // 临时任务队列
	handling_tasks = true;
	{
		lock_guard<mutex> lock(mtx);
		temp.swap(tasks);
	}

	for (const auto& cb : temp) { cb(); }
	handling_tasks = false;
}

void Reactor::AddTask(function<void()> cb) // trace("AddTask! InIOThread = ", InIOThread() ? "true" : "false")
{
	{
		lock_guard<mutex> lock(mtx);
		tasks.push_back(move(cb));
	}
	if (!InIOThread() || handling_tasks) { poller.WakeUp(); }
}

// 如果是IO线程，直接执行，非IO线程，放入IO队列，避免陷入死循环调用
void Reactor::Execute(function<void()> cb)
{
	if (InIOThread()) { cb(); }
	else { AddTask(move(cb)); }
}