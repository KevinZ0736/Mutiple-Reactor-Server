// trivial_test
// Created by kiki on 2021/12/5.20:27
// ./web_bench -c 1000 -t 6  http://127.0.0.1:5678/
//  ab -c1000 -n100000 http://127.0.0.1:5678/
#include "../TcpServer/TcpServer.h"

void conn_info(shared_ptr<TcpConnection> conn)
{
	if (conn->IsClose())
	{
		trace("连接 ", conn->connection_name, " 将要关闭！")
	}
	else
	{
		trace("建立了名为 ", conn->connection_name, " 的连接！", "服务器地址为 ", conn->self_addr.IpPortString(), " 对等方地址为 ", conn->peer_addr.IpPortString())
	}
}
void serve(const shared_ptr<TcpConnection>& conn)
{
	string str = conn->read_buffer.RetrieveAsString();
	conn->SendAcrossThreads(str.data(), str.size());
	conn->Close();
}

// telnet 127.0.0.1 5678
// nc 127.0.0.1 5678
int main()
{
	TcpServer server(Sockaddr_in(5678), "trivial服务器");

	server.SetConnectionCallback(conn_info);
	server.SetMessageCallback(serve);

	server.start();
}