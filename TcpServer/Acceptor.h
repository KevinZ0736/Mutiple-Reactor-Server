#include "../include/head/NoCopyable.h"
#include "File_Operation.h"
#include "FdGuard.hpp"
#include "EventRegister.h"
#include "EpollPoller.h"
#include "Sockaddr_in.hpp"
#include "console_log.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
// 前置声明，解决循环依赖
class Sockaddr_in;
class EpollPoller;

class Acceptor : NoCopyable
{
private:
	const FdGuard listen_fd;
	EventRegister new_conn_register; // 可读事件到来时接收连接，并执行下面的new_conn_callback。

	FdGuard dummy_fd; // 处理EMFILE

private:
	// 绑定连接的回调函数，参数为socket与socket的绑定对象
	function<void(int conn_fd, const Sockaddr_in& peer_addr)> new_conn_callback;
	// 被TcpServer设置为TcpServer::EstablishNewConnection。Acceptor将accept得到的conn_fd和对等方的地址传入该回调函数。
public:
	void SetNewConnCallback(function<void(int conn_fd, const Sockaddr_in& peer_addr)> cb) { new_conn_callback = move(cb); }

public:
	Acceptor(EpollPoller& epoller, const Sockaddr_in& listen_address);
	~Acceptor() = default;

	void start_listen();
};