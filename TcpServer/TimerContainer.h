#pragma once

#include "../include/head/NoCopyable.h"
#include "FdGuard.hpp"
#include "Timer.hpp"
#include "EventRegister.h"
#include "File_Operation.h"
#include "console_log.hpp"
// 定时器的目的是在预定的时间点执行某项任务，
// 例如在一段时间后执行某项操作、定期执行某个任务，或者在特定的日期和时间触发事件。

class TimerContainer :NoCopyable
{
private:

	// 在一个时间戳上可能有多个超时的定时器，因此不能用map。
	multimap<TimeStamp, unique_ptr<Timer>> timer_map;
	set<Timer*> canceling_timer_set;
	// 假设某一时刻a，b两个重复启动的定时器都超时，都被移到了expired vector中，先执行a的回调函数，紧接着就去执行b的回调函数，而b的回调函数中就是移除a定时器。
	// b的回调函数在timer_map中找不到a，于是将a添加到canceling_timer_set中。）
	// 再次遍历expired，如果其中的定时器是重复触发的，且不在canceling_timer_set中，再次计算其超时时刻，并插入timer_map中。
private:

	FdGuard timer_fd;
	EventRegister timerfd_register;
	void HandleRead(); // 查找timer_map,寻找到时事件,timerfd_register注册的响应函数。该函数应该在IO线程中被调用

private:
	bool calling_expired_timers {false}; //是否调用过期计时器

public:
	void AddTimer(function<void()> cb, TimeStamp when, double interval, Timer*& timer); // 添加Timer的函数，在insert上包装，不必加锁，只会被拥有TimerContainer的IO线程调用
	void CancelTimer(Timer* timer); // 不必加锁，只会被拥有TimerContainer的IO线程调用

	bool insert(unique_ptr<Timer> timer); //用于向一个定时器容器中插入一个定时器，并判断插入后是否改变了最早到期的定时器时间。 应当在IO线程中被调用

public:
	explicit TimerContainer(EpollPoller& poller);
	~TimerContainer() = default;


};

