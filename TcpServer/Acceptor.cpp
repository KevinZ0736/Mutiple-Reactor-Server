#include "Acceptor.h"

Acceptor::Acceptor(EpollPoller& epoller, const Sockaddr_in& listen_address, CLogFile& logfile)
	: listen_fd(open_socket()), new_conn_register(epoller, listen_fd.fd), dummy_fd(open_null())
{
	int ret = ::bind(listen_fd.fd, (struct sockaddr*)&listen_address.addr, static_cast<socklen_t>(sizeof(struct sockaddr_in)));
	if (ret == -1) { perror("Acceptor::ctor::bind"); exit(EXIT_FAILURE); }
	
	// 设置读取连接请求的 回调函数
	new_conn_register.SetReadCallBack
	(
		[this]()
		{
			Sockaddr_in peer_addr{};
			auto len = static_cast<socklen_t>(sizeof peer_addr.addr);
			int conn_fd = accept4(listen_fd.fd, (struct sockaddr*)&peer_addr.addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
			
			if (conn_fd != -1)  // 连接成功
			{
				if (new_conn_callback) { new_conn_callback(conn_fd, peer_addr); }
				else { close(conn_fd); }
			}
			else
			{
				// "EMFILE" 错误表示打开的文件描述符数目达到了系统限制
				if (errno == EMFILE)
				{
					close(dummy_fd.fd);
					dummy_fd.fd = accept(listen_fd.fd, NULL, NULL);
					close(dummy_fd.fd);
					dummy_fd.fd = open_null();

					logfile.Write("accept: EMFILE");
				}
			}
		}
	);

	//start_listen();
}

void Acceptor::start_listen(CLogFile& logfile)
{
	int ret = listen(listen_fd.fd, SOMAXCONN);
	if (ret == -1) { perror("Acceptor::start_listen::listen"); exit(EXIT_FAILURE); } // 可能是端口被占用

	new_conn_register.InterestReadableEvent(); 
	logfile.write("Acceptor开始监听连接！！！");
}