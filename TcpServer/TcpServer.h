#pragma once
#include "../include/head/NoCopyable.h"
#include "head/cpp_container.h"
#include "TcpConnection.h"
#include "Acceptor.h"
#include "IOThreadPool.h"
#include "console_log.hpp"

class TcpServer : NoCopyable
{
private:
	Reactor main_reactor; // 该变量的声明必须先于acceptor和IO_threadpool。main_reactor可以由主线程传递，而不在TcpServer中创建。
	IOThreadPool IO_threadpool; // sub reactors

public:
	Reactor& GetReactor() { return main_reactor; } // 应用层程序可获取main reactor，向其中添加任务

private:
	Sockaddr_in listen_addr; // 该变量用于初始化acceptor, 必须在acceptor前声明
	const string name; // 服务器名
	const string port; // 服务器端口

public:
	[[nodiscard]] string_view Port() const { return port; }
	[[nodiscard]] string_view Name() const { return name; }

private:
	Acceptor acceptor;
	int next_connId{ 1 };
	void EstablishNewConnection(int conn_fd, const Sockaddr_in& peer_addr); // 建立一个新的连接，只在IO线程中调用

private:
	map<string, shared_ptr<TcpConnection>> connection_map; // 连接列表： 连接名-连接对象的指针

private: // 这里的shared_ptr以值传递或以引用传递都可以,不影响引用计数。
	function<void(shared_ptr<TcpConnection>)> connection_callback; // 应用层注册的，连接建立后的回调函数。该回调被设置给TcpConnection对象。
	function<void(shared_ptr<TcpConnection>)> message_callback; // 应用层注册的，消息（数据）到来时的回调函数。该回调被设置给TcpConnection对象。
	function<void(shared_ptr<TcpConnection>)> writeComplete_callback; // 数据发送完毕时的回调函数。该回调被设置给TcpConnection对象。

public:
	void SetConnectionCallback(function<void(shared_ptr<TcpConnection>)> cb) { connection_callback = move(cb); }
	void SetMessageCallback(function<void(shared_ptr<TcpConnection>)> cb) { message_callback = move(cb); }
	void SetWriteCompleteCallback(function<void(shared_ptr<TcpConnection>)> cb) { writeComplete_callback = move(cb); }

private:
	void RemoveConnection(shared_ptr<TcpConnection> conn); // 这个函数是线程安全的

public:
	TcpServer(const Sockaddr_in& _listen_addr, string nm, size_t sub_reactor_count = 15);
	~TcpServer(); // 关闭所有的连接

	void start() { acceptor.start_listen(); debug("listen at ", listen_addr.IpPortString()) main_reactor.React(); } // main_reactor.Execute([this] { acceptor.start_listen(); });


};

void ignore_SIGPIPE();