#include "EventRegister.h"
#include "EpollPoller.h"

void EventRegister::Update() { poller.Register(this); } // 在Epoll中注册该事件
void EventRegister::Remove()
{
	if (!MonitoringNothing()) { err("将一个关注着事件的EventRegister移出了Poller！！！") }
	poller.Unregister(this);
}


void EventRegister::HandleEvent()
{
	/*这部分代码，首先尝试将 hold_ptr 提升为 std::shared_ptr<void>，并将结果赋值给 guard。
	通过 lock() 函数，我们可以观测 hold_ptr 是否有效，如果有效，则 guard 将指向 TcpConnection 对象，表示该对象还活着。
	如果无效，说明 TcpConnection 对象已经被释放，函数直接返回，不再处理后续的事件回调*/

	shared_ptr<void> guard;
	if (held) { guard = hold_ptr.lock(); if (!guard) { return; } }

	//  EPOLLIN：表示文件描述符可以读取（读事件）。
	//  EPOLLPRI：表示有紧急数据可读。
	//  EPOLLRDHUP：表示对等方关闭连接或半连接。
	//  EPOLLHUP：表示文件描述符挂起。
	//  EPOLLERR：表示发生错误
	if (triggered_events & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) { if (read_callback) { read_callback(); } } // EPOLLRDHUP : 对等方关闭连接或半连接。对该事件的处理应放在POLLIN前。否则POLLIN会处理该事件。
	else if (triggered_events & EPOLLOUT) { if (write_callback) { write_callback(); } }
	else if ((triggered_events & EPOLLHUP) && !(triggered_events & EPOLLIN)) { if (connection_callback) { connection_callback(); } }
	else if (triggered_events & (EPOLLERR)) { if (error_callback) { error_callback(); } }
}