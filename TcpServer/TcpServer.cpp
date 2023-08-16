#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "TcpServer.h"

void ignore_SIGPIPE()
{
	struct sigaction action {};
	action.sa_handler = SIG_IGN;
	action.sa_flags = 0;
	int ret = sigemptyset(&action.sa_mask);
	//调用 sigemptyset 函数，将 action 结构体中的信号屏蔽集合 sa_mask 初始化为空集。这样，在处理信号时不会阻塞其他信号。
	if (ret == -1) { perror("ignore_SIGPIPE::sigemptyset"); exit(EXIT_FAILURE); }
	ret = sigaction(SIGPIPE, &action, NULL);
	//调用 sigaction 函数，将 SIGPIPE 信号的处理设置为之前定义的 action 结构体。SIGPIPE 信号在网络编程中常常发生在远程连接关闭后继续发送数据的情况，忽略它可以避免程序异常终止
	if (ret == -1) { perror("ignore_SIGPIPE::sigaction"); exit(EXIT_FAILURE); }
}


void TcpServer::EstablishNewConnection(int conn_fd, const Sockaddr_in& peer_addr) // 只在IO线程中调用
{
	if (!main_reactor.InIOThread()) { fatal("EstablishNewConnection未在main_reactor线程中执行！") }

	char buf[32];
	snprintf(buf, sizeof buf, ":-%s#%d", port.data(), next_connId);
	++next_connId;
	string conn_name = name + buf;

	Reactor& reactor = IO_threadpool.GetReactor();  // 分配一个IO线程给新的连接，轮询分配以实现一定的负载均衡
	shared_ptr<TcpConnection> conn(make_shared<TcpConnection>(reactor, conn_name, conn_fd, listen_addr, peer_addr)); // 建立一个新的TcpConnection
	conn->SetConnCallback(connection_callback);
	conn->SetMessageCallback(message_callback);
	conn->SetWriteCompleteCallback(writeComplete_callback);
	conn->SetCloseCallback([this](auto&& PH1) { RemoveConnection(std::forward<decltype(PH1)>(PH1)); });

	connection_map[conn_name] = conn; // 把连接指针放入红黑树中

	reactor.Execute([conn] { conn->ConnectionEstablished(); }); // 执行，让event_register持有connnection资源
}

void TcpServer::RemoveConnection(shared_ptr<TcpConnection> conn)
{
	main_reactor.Execute // 单个reactor时，直接调用下面的函数
	(
		[this, conn]() // fixme:这里能不能捕获conn的引用？
		{
			connection_map.erase(conn->connection_name); // 只被main_reactor线程调用，无需加锁。
			conn->GetReactor().AddTask([conn] { conn->OnDestroyConnection(); });
		}
	);
}

TcpServer::TcpServer(const Sockaddr_in& _listen_addr, string nm, size_t sub_reactor_count)
	: listen_addr(_listen_addr), port(listen_addr.PortString()),
	name(move(nm)),
	IO_threadpool(main_reactor, sub_reactor_count),
	acceptor(main_reactor.GetPoller(), listen_addr)  // 主线程负责acceptor
{
	/*std::forward 用于实现完美转发，确保在函数调用中保持参数的值类别，避免不必要的拷贝和移动。
		&& （右值引用）用于实现移动语义，支持高效的资源管理和转移。
		std::move 用于将对象转换为右值引用，从而支持移动语义。*/

	acceptor.SetNewConnCallback([this](auto&& PH1, auto&& PH2) { EstablishNewConnection(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
	// 给accept建立回调函数
	ignore_SIGPIPE();
}

TcpServer::~TcpServer()
{
	for (auto& [conn_name, s_ptr] : connection_map)
	{
		shared_ptr<TcpConnection> conn = s_ptr;
		s_ptr.reset();
		conn->GetReactor().Execute([conn] { conn->OnDestroyConnection(); });
	}
}