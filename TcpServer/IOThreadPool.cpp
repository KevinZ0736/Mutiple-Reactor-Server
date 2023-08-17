#include "IOThreadPool.h"

void IOThreadPool::Start()
{
	if (thread_count == 0) { return; } // 这个判断很重要，否则线程会阻塞在cv.wait(ulock);

	for (size_t i = 0; i < thread_count; ++i)
	{
		thread_vector.emplace_back // thread_vector[i]线程所拥有的线程对象不一定是sub_reactor_vector[i]
		(
			[this]()
			{
				Reactor reactor; // 这里可以创建栈上的对象。

				{
					lock_guard<mutex> gurad(mtx); // 此处必须加锁，否则会出现double free or corruption (out) Aborted。 详见test中的测试。

					this->sub_reactor_vector.push_back(&reactor);
					if (this->sub_reactor_vector.size() == this->thread_count) { this->cond.notify_one(); }  // 保持创建Reactor与创建线程的同步
				}

				reactor.React(); // 让Reactor处理触发的事件
			}
		);
	}

	unique_lock<mutex> ulock(mtx);
	cond.wait(ulock, [this]() {return sub_reactor_vector.size() == thread_count; }); // 等待所有IO线程都创建完毕,再结束
}

IOThreadPool::IOThreadPool(Reactor& reactor, size_t count) : main_reactor(reactor), thread_count(count) // trace("IO线程池将要调用Start()！")
{
	thread_vector.reserve(count);
	sub_reactor_vector.reserve(count);
	Start();
	print();
}

IOThreadPool::~IOThreadPool() // log("开始执行IOThreadPool的析构函数！") log("IOThreadPool的析构函数执行完毕！")
{
	for (int i = 0; i < thread_count; ++i)
	{
		sub_reactor_vector[i]->quit();
	}
	for (int i = 0; i < thread_count; ++i)
	{
		thread_vector[i].join();
	}
}

// 当有子反应器时，它会循环使用不同的子反应器以实现负载均衡；如果没有子反应器，则会返回主反应器。
Reactor& IOThreadPool::GetReactor()
{
	if (sub_reactor_vector.empty()) { return main_reactor; }

	Reactor* es = sub_reactor_vector[index];

	++index;
	if (index == sub_reactor_vector.size()) { index = 0; }

	return *es;
}

void IOThreadPool::print()
{
	if (sub_reactor_vector.size() != thread_count) { fatal("print") }
	if (thread_vector.size() != thread_count) { fatal("print") }
	for (int i = 0; i < thread_count; ++i)
	{
		log(thread_vector[i].get_id(), "-----", sub_reactor_vector[i]->GetTid())
	}
	trace("创建IO线程池完毕！")
}