#include "../include/head/NoCopyable.h"
#include "head/cpp_container.h"
#include "Reactor.h"
#include "console_log.hpp"

class IOThreadPool :NoCopyable
{
private:
	mutex mtx; // 这两个变量的声明最好放在最前面
	condition_variable cond;

private:
	size_t thread_count; // 线程数量
	int index{ 0 };      // 线程索引

public:
	[[nodiscard]] size_t ThreadCount() const { return thread_count; }
	[[nodiscard]] int Index() const { return index; }

private:
	// 主线程中创建的Reactor对象, 处理Acceptor的IO事件，
	// 即accept。main_reactor必须由主线程创建，不能从IO线程池获取，否则主线程将不执行任何任务。
	Reactor& main_reactor;

private:
	vector<thread> thread_vector;
	// 处理TcpConnection的IO事件。对于IO密集型的服务，轻量的计算任务可由IO线程代为处理。而计算密集型的服务，应交由计算线程池处理。
	vector<Reactor*> sub_reactor_vector;

public:
	void Start(); // 在线程函数的栈上创建reactor, 并调用reactor.React();
	explicit IOThreadPool(Reactor& reactor, size_t count = thread::hardware_concurrency());
	~IOThreadPool(); //调用Reactor->quit()函数，结束循环，从而让线程退出

public:
	Reactor& GetReactor(); // 以轮叫（roundbin）的方式分配sub reactor；该函数只会被main reactor中的acceptor调用，不需要加锁。
	void print(); // 仅作debug用。用于显示thread_vector[i]线程所拥有的线程对象不一定是sub_reactor_vector[i]

};