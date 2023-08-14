#include <unistd.h> // close

// 文件描述符的封装，将fd隐转int
struct FdGuard
{
	int fd;

	explicit FdGuard(int f) : fd(f) { }

	explicit operator int() { return fd; }

	~FdGuard() { close(fd); }
};