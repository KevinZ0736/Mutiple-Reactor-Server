#include "EpollPoller.h"

void EpollPoller::poll(int ms_timeout, CLogFile& logfile)
{
	int active_events = epoll_wait(epoll_fd.fd, events_vector.data(), static_cast<int>(events_vector.size()), ms_timeout);

	if (active_events > 0)
	{
		for (int i = 0; i < active_events; ++i)
		{
			/*在 epoll 多路复用模型中，epoll_event.data.ptr 的作用是让用户在注册事件时，可以将自定义的数据与事件进行关联。
			  当事件发生时，我们可以通过获取 epoll_event.data.ptr 来获取之前设置的用户数据指针，
			  然后根据这个用户数据指针，进行相应的数据读取或其他操作。*/
			auto er = static_cast<EventRegister*>(events_vector[i].data.ptr);
			er->SetTriggeredEvents(static_cast<int>(events_vector[i].events));
			active_registers.push_back(er);
		}

		if (static_cast<size_t>(active_events) == events_vector.size())
		{
			events_vector.resize(events_vector.size() * 2);  // 存储事件容器 扩容
		}
	}
	else if (active_events == 0) { logfile.Write("epoll_wait阻塞等待了", ms_timeout, "毫秒，仍没有感兴趣的事件发生，epoll_wait返回0"); }
	else if (active_events == -1) { perror("EpollPoller::poll::epoll_wait"); }
}

void EpollPoller::HandleActiveEvents()
{
	EpollPoller::poll();
	for (EventRegister* event_register : active_registers)
	{
		event_register->HandleEvent();
	}
	active_registers.clear();
}

void EpollPoller::epoll_control(int operation, EventRegister* er) const
{
	struct epoll_event event {};
	event.events = er->Events();
	event.data.ptr = er;

	int ret = epoll_ctl(epoll_fd.fd, operation, er->Fd(), &event);
	if (ret == -1) { perror("EpollPoller::epoll_control::epoll_ctl"); }
}

void EpollPoller::Register(EventRegister* er) const
{
	const int index = er->State();
	if (index == first_register || index == unregistered) { epoll_control(EPOLL_CTL_ADD, er); er->SetState(registered); }
	else // state = registered
	{
		//查看注册的事件是否是空事件，是的话就删除，不是就删除
		if (er->MonitoringNothing()) { epoll_control(EPOLL_CTL_DEL, er); er->SetState(unregistered); }
		else { epoll_control(EPOLL_CTL_MOD, er); }
	}
}

void EpollPoller::Unregister(EventRegister* er) const
{
	if (er->State() == registered) { epoll_control(EPOLL_CTL_DEL, er); }
	er->SetState(first_register); // er->SetState(unregistered);
}

EpollPoller::EpollPoller() : epoll_fd(open_epoll()), events_vector(16), wakeup_fd(open_eventfd()), wakeup_register(*this, wakeup_fd.fd)
{
	wakeup_register.SetReadCallBack
	(
		[this]()
		{
			unsigned long long one = 1;
			ssize_t ret = read(wakeup_fd.fd, &one, sizeof one);
			if (ret != sizeof one) { perror("EpollPoller::wakeup_register.SetReadCallBack::read"); }
		}
	);
	wakeup_register.InterestReadableEvent();
}