// trivial_test
// Created by kiki on 2021/12/6.16:00
//	./web_bench -c 10000 -t 60  http://127.0.0.1:5678/
// ab -c1000 -n100000 http://127.0.0.1:5678/
#include "../HttpServer/HttpServer.hpp"
#include <signal.h>

int conn_count = 1;

int hammingWeight(unsigned int n) // 返回十进制数字n转化为二进制后，1的个数
{
	int ret = 0;
	while (n)
	{
		n &= n - 1;
		ret++;
	}
	return ret;
}

int main()
{
	signal(SIGPIPE,SIG_IGN);
	HttpServer server(Sockaddr_in(5678), "tirvial http server", 7);
	server.add_servlet
	(
		"/",
		[](HttpRequest& req, HttpResponse& res)
		{
			++conn_count;
			res.Body() = to_string(hammingWeight(conn_count));
			return true;
		}
	);
	server.start();
}
