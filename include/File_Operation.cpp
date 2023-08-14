#include"File_Operation.h"


int open_socket()
{
	// SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC：指定套接字类型为流式（TCP），
	// 同时设置非阻塞模式和关闭执行操作时关闭套接字（CLOEXEC）标志。
	// 非阻塞模式意味着套接字操作不会阻塞程序的执行，而 CLOEXEC 标志会在执行 exec 类函数时自动关闭套接字
	int fd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
	if (fd == -1) { perror("open_socket::socket"); exit(EXIT_FAILURE); }

	int opt = 1;
	// 设置套接字选项，具体来说是启用了地址重用选项
	int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, static_cast<socklen_t>(sizeof opt));
	if (ret == -1) { perror("open_socket::setsockopt"); exit(EXIT_FAILURE); }

	return fd;
}

//EPOLL_CLOEXEC：设置此标志会在执行新程序时自动关闭创建的 epoll 文件描述符。
//这对于避免文件描述符泄漏在某些情况下非常有用。
int open_epoll()
{
	int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (epoll_fd == -1) { perror("open_epoll::epoll_create1"); exit(EXIT_FAILURE); }

	return epoll_fd;
}

//这段代码定义了一个名为 open_null 的函数，用于打开 /dev/null 文件，
//并以只读模式和 O_CLOEXEC 标志打开。如果打开文件失败，函数将使用 perror 打印错误信息并退出程序，否则返回文件描述符。
int open_null()
{
	int ret = open("/dev/null", O_RDONLY | O_CLOEXEC);
	if (ret == -1) { perror("open_null::open"); exit(EXIT_FAILURE); }
	return ret;
}

int open_eventfd()
{
	int event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (event_fd == -1) { perror("open_eventfd::eventfd"); exit(EXIT_FAILURE); }
	return event_fd;
}

/*
sleep / usleep / alarm 在实现时有可能用了信号，在多线程程序中处理信号是相当麻烦的，应当尽量避免。
getitimer和timer_create也是用信号来deliver超时。timer_create可以指定信号的接收方是线程还是进程，这是一个进步，但在信号处理函数中能做的事很有限。
nanosleep和clock_nanosleep会让线程挂起，在非阻塞网络编程中不能让线程挂起，应当注册一个时间回调函数。
timerfd_create把时间变成一个文件描述符，该文件在定时器超时的那一刻变得可读，可以用统一的方式处理IO事件和超时事件。
*/

int open_timerfd()
{
	//CLOCK_MONOTONIC 表示一个单调递增的时钟，不受系统时间的影响，适用于测量时间间隔等操作。
    //TFD_NONBLOCK 和 TFD_CLOEXEC：这是用于创建文件描述符的标志。TFD_NONBLOCK 表示将文件描述符设置为非阻塞模式，
	//TFD_CLOEXEC 表示在执行 exec() 系列函数时关闭文件描述符。
	
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC); // CLOCK_MONOTONIC表示用户手动修改系统时间，不会影响到timerfd。
	if (timer_fd == -1) { perror("open_timerfd::timerfd_create"); exit(EXIT_FAILURE); }
	return timer_fd;
}

void read_timerfd(int timerfd)
{
	unsigned long long data;
	ssize_t n = read(timerfd, &data, sizeof data);
	if (n != sizeof data) { perror("read_timerfd::read"); exit(EXIT_FAILURE); }
}

void reset_timerfd(int timerfd, long expiration) // 设置新的超时时间。expiration为超时时间点。
{
	struct itimerspec new_value {};
	struct itimerspec old_value {}; // 输出参数

	long us = expiration - TimeStamp::Now().UsSinceEpoch(); // 经过us秒后超时。
	if (us < 100) { us = 100; }
	struct timespec ts;
	//timespec 适用于更广泛的 POSIX 时间操作，而 timeval 更倾向于在套接字和部分早期 UNIX 环境中使用。在现代的 C++ 编程中，特别是在高精度时间操作或等待超时的情况下，timespec 更常见。
	ts.tv_sec = static_cast<time_t>(us / 1000'000);
	ts.tv_nsec = static_cast<long>((us % 1000'000) * 1000); // (us % 1000'000) * 1000完全不等于 us % 1000
	new_value.it_value = ts;

	int ret = timerfd_settime(timerfd, 0, &new_value, &old_value); // 设置新的超时时间，将旧的超时时间保存在old_value中。
	if (ret == -1) { perror("reset_timerfd::timerfd_settime"); exit(EXIT_FAILURE); }
}