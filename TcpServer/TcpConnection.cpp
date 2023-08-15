#include "TcpConnection.h"

TcpConnection::TcpConnection(Reactor& re, string nm, int connection_fd, const Sockaddr_in& self, const Sockaddr_in& peer)
	: connection_name(move(nm)),
	reactor(re), conn_fd(connection_fd),
	event_register(re.GetPoller(), conn_fd.fd),
	self_addr(self), peer_addr(peer)
{
	event_register.SetReadCallBack([this] { HandleRead(); });
	event_register.SetWriteCallBack([this] { HandleWrite(); });
	event_register.SetCloseCallBack([this] { HandleClose(); });
	event_register.SetErrorCallBack([this] { HandleError(); }); // info("创建了一个TcpConnection对象, 对等方地址为 ", peer_addr.IpPortString(), "  服务器地址为 ", self_addr.IpPortString())

	int opt = 1;
	int ret = ::setsockopt(conn_fd.fd, SOL_SOCKET, SO_KEEPALIVE, &opt, static_cast<socklen_t>(sizeof opt));
	//KeepAlive定期探测连接是否存在，如果应用层有心跳，这个选项就不必设置
	if (ret == -1) { perror("TcpConnection::ctor::setsockopt"); }
}

// 从连接fd中读取数据
void TcpConnection::HandleRead()
{
	ssize_t n = read_buffer.ReadFromFd(conn_fd.fd);
	if (n == 0) { HandleClose(); }
	else if (n < 0) { perror("TcpConnection::HandleRead()"); HandleError(); }
	else { message_callback(shared_from_this()); }
	// 用户的message_callback可以把任务提交给计算线程池，计算完毕后（计算线程）调用TCPConnection::SendAcrossThreads(), 把结果发送给客户端。
}

// 向连接fd写入数据,在写事件发生时将数据从发送缓冲区写入内核发送缓冲区，并根据情况取消关注写事件。
void TcpConnection::HandleWrite()
{
	if (!reactor.InIOThread()) { fatal("不能够跨线程调用HandleWrite函数！") }
	if (!event_register.MonitoringWritable()) { fatal("call HandleWrite() but not MonitoringWritable") }
	ssize_t n = write(conn_fd.fd, write_buffer.readable_ptr(), write_buffer.ReadableBytes()); // 这里确实是readable_ptr()，而不是writable_ptr，写入数据后移动的是writable_ptr
	if (n <= 0) { perror("write return value <= 0"); } // write函数不会返回0，除非第三个参数指定为0。这一点与read函数不一样。

	write_buffer.Advance(n);
	if (write_buffer.ReadableBytes() == 0) // 应用层发送缓冲区已清空
	{
		event_register.UnInterestWritableEvent(); // 停止关注EPOLLOUT事件，以免出现busy loop.EPOLL ET模式只要在最开始关注EPOLLOUT事件即可。如果频繁触发EPOLLOUT事件，则用ET模式性能更好。
		if (write_complete_callback) { reactor.AddTask([this] { write_complete_callback(shared_from_this()); }); }
		if (state == ConnectionState::Disconnecting) { shutdown(conn_fd.fd, SHUT_WR); } // 这是一个系统调用，用于关闭套接字的写入端。conn_fd.fd 表示套接字描述符，SHUT_WR 是一个常量，表示关闭写入端。fixme:改写了muduo
	}
}

void TcpConnection::HandleClose()
{
	if (!reactor.InIOThread()) { fatal("不能够跨线程调用HandleClose函数！") } if (!(state == ConnectionState::Connected || state == ConnectionState::Disconnecting)) { fatal("TcpConnection异常地调用了HandleClose()！") }
	state = ConnectionState::Disconnected;

	event_register.UnInterestAllEvents();

	if (connection_callback) { connection_callback(shared_from_this()); }

	close_callback(shared_from_this());	// 调用TcpServer::removeConnection， 从TCPServer中移除TcpConnection。必须在最后一行。
}

void TcpConnection::HandleError() const
{
	int opt;
	auto len = static_cast<socklen_t>(sizeof opt);
	int ret = getsockopt(conn_fd.fd, SOL_SOCKET, SO_ERROR, &opt, &len);//这段代码的主要目的是获取套接字的错误状态（错误码）。
	if (ret == -1) { perror("TcpConnection::HandleError::getsockopt"); }
	else { fprintf(stderr, "TcpConnection::HandleError()--- %s \n", strerror(opt)); }
}

void TcpConnection::OnDestroyConnection() // 被TcpServer::removeConnection调用。
{
	if (!reactor.InIOThread()) { fatal("不能够跨线程调用！") }

	if (state == ConnectionState::Connected) // 可能调用了ShutDown函数？
	{
		state = ConnectionState::Disconnected;
		event_register.UnInterestAllEvents();
		if (connection_callback) { connection_callback(shared_from_this()); }
	}

	event_register.Remove();
}

void TcpConnection::ConnectionEstablished() // 连接建立时，被TcpServer加入到Reactor的任务队列中
{
	if (!reactor.InIOThread()) { fatal("不能够跨线程调用！") }
	if (state != ConnectionState::Connecting) { fatal("state != ConnectionState::Connecting！") }
	state = ConnectionState::Connected;

	event_register.hold(shared_from_this());   // event_register持有connnection资源
	event_register.InterestReadableEvent();    // 关注可读事件

	if (connection_callback) { connection_callback(shared_from_this()); }  // 执行连接成功后的回调函数
}


// 负责尝试将数据写入发送缓冲区并注册写事件,注意拆分写入缓冲区与填入内核的操作的用处
void TcpConnection::Send(const char* data, size_t len) // 尝试向内核缓冲区中写，内核缓冲区满后向应用层输出缓冲区中写。
{
	if (!reactor.InIOThread()) { fatal("不能够跨线程调用Send函数！") }
	if (state == ConnectionState::Disconnected) { err("call send at Disconnected TcpConnection") return; }
	if (state != ConnectionState::Connected) { return; }

	ssize_t wrote_bytes{ 0 };      // 已写数据，有符号数
	size_t remaining_bytes{ len };  // 剩余数据，无符号数
	if (write_buffer.ReadableBytes() == 0 && !event_register.MonitoringWritable()) // 内核发送缓冲区尚有空间，直接发送。
	{
		wrote_bytes = ::write(conn_fd.fd, data, len);
		if (wrote_bytes != -1)
		{
			remaining_bytes -= wrote_bytes;
			if (remaining_bytes == 0 && write_complete_callback)
			{
				reactor.AddTask([this] {write_complete_callback(shared_from_this()); });
			}
			//在write_complete_callback调用Send函数，向客户端发送数据（见CharacterGenerator.cpp），
			//如果是reactor.Execute（...）而不是reactor.AddTask(...)，则会循环调用Send()和write_complete_callback，进而爆栈。
		}
		else if (errno == EWOULDBLOCK) { wrote_bytes = 0; }
		else { perror("TcpConnection::Send::write"); return; }
	}

	if (remaining_bytes > 0)
	{
		size_t old_len = write_buffer.ReadableBytes();
		if (old_len < buffer_full_size && old_len + remaining_bytes >= buffer_full_size && buffer_full_callback) // 如果上一次已经触发过buffer_full_callback，但没有得到处理，这一次就不再触发。
		{
			reactor.AddTask([this, old_len, remaining_bytes]() { buffer_full_callback(shared_from_this(), old_len + remaining_bytes); });
		}

		write_buffer.Append(data + wrote_bytes, remaining_bytes);
		if (!event_register.MonitoringWritable()) { event_register.InterestWritableEvent(); }
	}
}

void TcpConnection::SendAcrossThreads(const char* data, size_t len) // 计算线程池中的计算线程处理计算密集型任务，得到结果后跨线程调用Send函数。
{
	shared_ptr<TcpConnection> guard(shared_from_this());
	reactor.AddTask([guard, data, len] { guard->Send(data, len); });
	//reactor.AddTask([this, data, len] { Send(data, len); } );
}


void TcpConnection::ShutDown()
{
	if (!reactor.InIOThread()) { fatal("不能够跨线程调用ShutDown函数！") }
	if (state != ConnectionState::Connected) { return; }
	state = ConnectionState::Disconnecting;
	if (event_register.MonitoringWritable()) { return; } // conn->send(file)；conn->shutdown()；像这样的调用是安全的。 如果还在发送数据，就不应该关闭写端。同时TcpConnection::ShutDown()并不是不做任何事，它将状态改为Disconnecting。当内核缓冲区中的数据发送完毕之后，可写事件产生，调用handleWrite函数，该函数中又调用了shutdown

	int ret = shutdown(conn_fd.fd, SHUT_WR);
	if (ret == -1) { perror("shutdown"); }
}

void TcpConnection::ShutDownAcrossThreads()
{
	shared_ptr<TcpConnection> guard(shared_from_this());
	reactor.AddTask([guard] { guard->ShutDown(); });
	//reactor.AddTask( [this] { ShutDown(); } );
}
void TcpConnection::Close()
{
	if (state == ConnectionState::Disconnected || state == ConnectionState::Connecting) { return; }
	state = ConnectionState::Disconnecting;
	shared_ptr<TcpConnection> guard(shared_from_this());
	reactor.AddTask([guard]() {guard->HandleClose(); });
}