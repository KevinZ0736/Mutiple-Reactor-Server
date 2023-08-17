#pragma once
#include "../include/head/NoCopyable.h"
#include "../include/head/cpp_smartptr.h"
#include "ApplicationBuffer.hpp"
#include "FdGuard.hpp"
#include "Sockaddr_in.hpp"
#include "console_log.hpp"
#include "EventRegister.h"
#include "Reactor.h"


// 模板类 enable_shared_from_this 的使用。
// 它允许一个对象在被智能指针管理的情况下，获得一个指向自身的 shared_ptr。
class TcpConnection : private NoCopyable, public enable_shared_from_this<TcpConnection>
{
public:
	const string connection_name;

private:
	enum class ConnectionState // todo:使用compare and swap, 即原子操作
	{
		Connecting, // 初始状态
		Connected, // 调用了ConnectionEstablished后
		Disconnecting, // 调用了close或shutdown, 但应用层发送缓冲区尚有未发送完的数据
		Disconnected // 调用了close或shutdown, 关闭了连接
	};
	ConnectionState state{ ConnectionState::Connecting };
public:
	bool IsClose() const { return state == ConnectionState::Disconnected; }

private:
	FdGuard conn_fd;
	EventRegister event_register;
	Reactor& reactor;

public:
	Reactor& GetReactor() { return reactor; }

public:
	const Sockaddr_in self_addr;
	const Sockaddr_in peer_addr;

public:
	ApplicationBuffer read_buffer; // 接收缓冲区。将对等方发来的数据从内核缓冲区拷贝到接收缓冲区来。
	ApplicationBuffer write_buffer; // 发送缓冲区。数据直接发给内核缓冲区，内核缓冲区满后在暂存在这个应用层缓冲区中。
	any context; // TCP连接上下文。应用举例：分块发送大文件，连接建立时用context保存文件指针。

private:
	function<void(shared_ptr<TcpConnection>)> connection_callback; // 用户设置的，连接建立和断开时执行的回调函数。
	function<void(shared_ptr<TcpConnection>)> message_callback; // 用户设置的，消息到来后执行的回调函数。
	function<void(shared_ptr<TcpConnection>)> write_complete_callback; // 用户设置的，消息发送完毕后执行的回调函数。
	size_t buffer_full_size{ 64 * 1024 * 1024 };
	function<void(shared_ptr<TcpConnection>, size_t)> buffer_full_callback;
	function<void(shared_ptr<TcpConnection>)> close_callback; // 被TcpServer设置为TcpServer::RemoveConnection。
public:
	void SetConnCallback(function<void(shared_ptr<TcpConnection>)> cb) { connection_callback = move(cb); }
	void SetMessageCallback(function<void(shared_ptr<TcpConnection>)> cb) { message_callback = move(cb); }
	void SetWriteCompleteCallback(function<void(shared_ptr<TcpConnection>)> cb) { write_complete_callback = move(cb); }
	void SetBufferFullCallback(function<void(shared_ptr<TcpConnection>, size_t)> cb) { buffer_full_callback = move(cb); }
	void SetCloseCallback(function<void(shared_ptr<TcpConnection>)> cb) { close_callback = move(cb); }

private:
	void HandleRead(); // 这四个成员函数被注册给event_register
	void HandleWrite();  // 写事件的回调函数，将数据缓冲区的剩余数据发送，关闭写事件
	void HandleClose();
	void HandleError() const;
public:
	void OnDestroyConnection();    // 关闭该Tcp连接
	void ConnectionEstablished();  // 建立Tcp连接
public:
	void Send(const char* data, size_t len); // Send() 函数尝试将数据直接发送到套接字的内核缓冲区。如果内核缓冲区有足够的空间，数据会被写入到内核缓冲区中。
	void SendAcrossThreads(const char* data, size_t len); // SendAcrossThreads() 函数是一种特殊情况，用于实现多线程环境下的数据发送。
public:
	void ShutDown();          // 关闭连接
	void ShutDownAcrossThreads();
	void Close(); // 可以跨线程调用

public:
	TcpConnection(Reactor& re, string nm, int connection_fd, const Sockaddr_in& self, const Sockaddr_in& peer);
	~TcpConnection() = default;


};