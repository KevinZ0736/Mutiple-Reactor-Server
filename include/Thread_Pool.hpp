#include"./head/NoCopyable.h"
#include"./head/cpp_container.h"
#include"./head/cpp_functional.h"
#include"./head/cpp_thread.h"
#include<atomic>
#include<iostream>
using std::runtime_error;
using namespace std;

class ThreadPool :NoCopyable
{
private:
	mutex m_mutex;
	condition_variable cond;    // 条件变量

	vector<thread> v_threads;   // 线程容器
	queue<function<void()>> tasks_queue;  // 任务队列

	atomic<bool> is_running{true};     // 线程池的操作状态

public:
	// 线程池中线程数量
	[[nodiscard]] size_t GetThreadCount() const { return v_threads.size(); }

	// 将构造函数用作自动类型转换函数似乎是一项不错的特性，但有时候会导致意外的类型转换。explicit关键字用于关闭这种自动特性，但仍允许显式转换。
	// 线程池是IO密集还是CPU密集 CPU密集型：核心线程数 = CPU核数 + 1 IO密集型：核心线程数 = CPU核数 * 2
	explicit ThreadPool(size_t count = thread::hardware_concurrency() + 2)
	{
		v_threads.reserve(count);
		for (size_t i = 0; i < count; ++i)
		{
			v_threads.emplace_back([this, count]() {
				while (is_running)
				{
					function<void()> task;
					{
						unique_lock<mutex> lock(m_mutex);
						while (tasks_queue.empty() || !is_running)
						{
							cond.wait(lock);
						}
						if (!tasks_queue.empty() && is_running)
						{
							task = move(tasks_queue.back());
							tasks_queue.pop();
						}
					}
					task();
				}
				});
		}

		print_thread();
	}

	// 添加任务
	auto add_task(function<void()> task)
	{
		{
			{
				lock_guard<mutex> lock(m_mutex);
				if (!is_running) { throw runtime_error("add task on stopped thread_pool"); }
				tasks_queue.emplace(move(task));
			}

			cond.notify_one();
		}
	}

	~ThreadPool()
	{

		{
			lock_guard<mutex> lock(m_mutex); // 思考：这里有必要加锁吗？
			is_running = false;
		}

		cond.notify_all();

		for (auto& thread : v_threads)
		{
			thread.join();
		}

	}

	// 打印池中线程
	void print_thread() const
	{
		size_t n = GetThreadCount();
		cout << "计算线程的数量是" << n << "它们的tid如下" << endl;
		for (int i = 0; i < n; ++i)
		{
			cout << v_threads[i].get_id() << endl;
		}
	}

};