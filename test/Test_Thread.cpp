#include "Thread_Pool.hpp"
#include <iostream>
#include <atomic>
using namespace std;

class A
{
public:
	atomic<int> seq = 0;
public:
	A()
	{
		cout << "constructor called !!!" << endl;
	}

	A(const A& a)
	{
		cout << "copy constructor called !!!" << endl;
	}

	void print()
	{
		cout << this_thread::get_id() << "  seq=" << ++seq << endl;
	}
};

int main()
{
	ThreadPool pool(3);
	A a;
	pool.add_task([&]() {a.print(); });
	for (int i = 0; i < 100; ++i)
	{
		pool.add_task(bind(&A::print, &a));
	}

	// while (a.seq < 7) { }
}
