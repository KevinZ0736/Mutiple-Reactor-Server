#pragma once
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include "./head/cpp_container.h"

struct Sockaddr_in
{
	struct sockaddr_in addr {};

	Sockaddr_in() = default;
	Sockaddr_in(const Sockaddr_in&) = default; // 拷贝构造
	Sockaddr_in& operator=(const Sockaddr_in&) = default; // 赋值运算符

	explicit Sockaddr_in(unsigned short port)  // 自动获取IP
	{
		addr.sin_family = PF_INET;                 // ipv4
		addr.sin_addr.s_addr = get_local_ip();     // 将ip地址赋值到addr结构体中
		addr.sin_port = htons(port);               // 设置端口号，并改为网络字节序
	}

	explicit Sockaddr_in(const struct sockaddr_in& a) : addr(a) {}

	Sockaddr_in(string_view ip, short port) // 指定IP和端口
	{
		addr.sin_family = PF_INET;
		int ret = inet_pton(PF_INET, ip.data(), &addr.sin_addr);
		if (ret != 1) { perror("Sockaddr_in::inet_pton"); exit(EXIT_FAILURE); }
		addr.sin_port = htons(port);
	}

	[[nodiscard]] unsigned int Ip() const { return addr.sin_addr.s_addr; }
	[[nodiscard]] unsigned short Port() const { return addr.sin_port; }

	[[nodiscard]] string PortString() const  // 以string方法返回Port
	{
		char buf[32] = { 0 };
		unsigned short port = ntohs(addr.sin_port);
		snprintf(buf, sizeof buf, "%d", port);
		return buf;
	}

	[[nodiscard]] string IpString() const  // 以string方法返回IP
	{
		char buf[32] = { 0 };
		const char* ret = inet_ntop(PF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(sizeof buf));
		if (ret == NULL) { perror("Sockaddr_in::IpString::inet_pton"); exit(EXIT_FAILURE); }
		return buf;
	}

	[[nodiscard]] string IpPortString() const // 以string返回IP 加 Port
	{
		return IpString() + ", " + PortString();
	}

	static in_addr_t get_local_ip()  // 自动获取本主机的IP地址
	{
		int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sockfd == -1) { perror("get_local_ip::socket"); return -1; }
		/*ifreq 结构体是用于在网络编程中进行网络接口配置和查询的数据结构。
		它通常用于与操作系统的网络接口进行交互，获取和设置网络接口的属性，如 IP 地址、MAC 地址、状态等信息。*/
		struct ifreq ireq {};
		strcpy(ireq.ifr_name, "eth0");

		// 使用 IOCTL 请求 SIOCGIFADDR 查询指定网络接口的 IP 地址，并将结果存储在 ireq 结构体中。
		ioctl(sockfd, SIOCGIFADDR, &ireq);

		auto* host = (struct sockaddr_in*)&ireq.ifr_addr;

		close(sockfd);

		return host->sin_addr.s_addr;
	}

};