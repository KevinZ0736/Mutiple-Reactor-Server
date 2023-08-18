#include <vector>
#include <array>
#include <string>
#include <string_view>
#include <algorithm>
using std::vector, std::array, std::string, std::string_view;
using std::swap, std::search, std::copy;
#include <string.h> // memchr
#include <errno.h>
#include <sys/uio.h>


static constexpr size_t reserved = 8; // 缓冲区头部预留8字节空间，以便应用层协议在头部写入消息长度等头部信息
static constexpr size_t initial_size = 1024;

class ApplicationBuffer
{
private:
	vector<char> buffer;
	size_t readable_index{ reserved }; // vector扩容后会使迭代器失效，此处不能记录指针。表示可读数据在缓冲区中的开始索引
	size_t writable_index{ reserved }; // 表示可写数据在缓冲区中的索引。
public: // peek, begin_write
	[[nodiscard]] const char* readable_ptr() const { return buffer.data() + readable_index; }
	[[nodiscard]] const char* writable_ptr() const { return buffer.data() + writable_index; }
	[[nodiscard]] auto readable_iter() const { return buffer.begin() + static_cast<long>(readable_index); }
	[[nodiscard]] auto writable_iter() const { return buffer.begin() + static_cast<long>(writable_index); }
public:
	[[nodiscard]] size_t ReadableBytes() const { return writable_index - readable_index; }  // 表示有多少可读数据
	[[nodiscard]] size_t WritableBytes() const { return buffer.size() - writable_index; }   // 表示还能写多少数据
	[[nodiscard]] size_t ReservedBytes() const { return readable_index - 0; }               // 可读索引与开头的距离

public:
	// 将数据追加到缓冲区的可写位置，并更新writable_index。
	void Append(const char* data, size_t len)
	{
		append_aux(len);
		copy(data, data + len, buffer.data() + writable_index); // std::copy通过type_traits判断要拷贝的数据类型，对于char，调用memmove，效率是很高的。
		writable_index += len;
	}
	// 将数据插入到缓冲区的可读位置，并更新readable_index。
	void PrePend(const char* data, size_t len)
	{
		readable_index -= len;
		copy(data, data + len, buffer.data() + readable_index);
	}
	// 返回指定长度的可读数据，但不会移动readable_index
	void Peek(char* buf, size_t len) const { memcpy(buf, readable_ptr(), len); }
	// 返回指定长度的可读数据，并移动readable_index。
	void Read(char* buf, size_t len) { Peek(buf, len); Advance(len); }

public:
	//  移动readable_index指针，用于标记已读取数据的部分。
	void Advance(size_t len) // 只是修改指针
	{
		if (len > ReadableBytes()) { return; }
		if (len < ReadableBytes()) { readable_index += len; }
		else { readable_index = reserved; writable_index = reserved; } // 将可读数据读完，指针置零
	}
	//  移动readable_index指针，用于标记已读取数据的部分。
	void Advance() { Advance(ReadableBytes()); }
private:
	//  在缓冲区中添加额外的容量以容纳更多数据。
	void append_aux(size_t len)
	{
		if (WritableBytes() >= len) { return; } // 容量足够，直接返回
		if (WritableBytes() + ReservedBytes() < reserved + len) // 容量不足，扩充容量
		{
			buffer.resize(writable_index + len);
		}
		else // 将buffer中原有的数据移到buffer前面，腾出后面的空间
		{
			size_t readable = ReadableBytes();
			copy(readable_ptr(), writable_ptr(), buffer.begin() + reserved);
			readable_index = reserved;
			writable_index = readable_index + readable;
		}
	}

public:
	//  在缓冲区中查找换行符\r\n，并返回其位置。
	const char* FindCRLF(const char* start = nullptr) const
	{
		array<char, 2> CRLF{'\r', '\n'};
		start = (start == nullptr ? readable_ptr() : start);
		auto iter = search(start, writable_ptr(), CRLF.begin(), CRLF.end()); // std::search的用法 https://zh.cppreference.com/w/cpp/algorithm/search
		return iter == writable_ptr() ? nullptr : iter;
	}
	//  在缓冲区中查找换行符\n，并返回其位置。
	const char* FindEOL(const char* start = nullptr) const
	{
		start = (start == nullptr ? readable_ptr() : start);
		return static_cast<const char*>(memchr(start, '\n', writable_ptr() - start)); // void *memchr(const void *buf, int ch, size_t count) 从buf所指内存区域的前count个字节查找字符ch。
	}
public:
	//  以字符串形式返回指定长度的可读数据，并移动readable_index。
	string RetrieveAsString(size_t len = -1) // 会读走数据
	{
		len = (len == -1 ? ReadableBytes() : len);
		string str(readable_ptr(), len);
		Advance(len);
		return str;
	}
	//  返回一个string_view对象，用于访问当前可读数据，但不会移动readable_index。
	[[nodiscard]] string_view StringView() const // 不会读走数据
	{
		return { readable_ptr(), ReadableBytes() };
	}

public:
	// 从文件描述符（fd）中读取数据，并将数据存储到缓冲区中，同时更新writable_index。返回从fd中读到的字节数，最多读取128k数据
	ssize_t ReadFromFd(int fd)
	{
		const size_t writable = WritableBytes();

		char extra_buf[65536]; // 临时的栈上空间

		// 用于在一个系统调用中传递多个非连续的缓冲区。
		struct iovec vec[2];
		vec[0].iov_base = buffer.data() + writable_index;
		vec[0].iov_len = writable;
		vec[1].iov_base = extra_buf;
		vec[1].iov_len = sizeof extra_buf;


		/*是为了将缓冲区的可写部分和额外的缓冲区（extra_buf）同时传递给 readv() 系统调用。
		  在这个代码中，readv() 被用来一次性从文件描述符 fd 中读取数据到多个缓冲区中。readv() 的参数是一个数组 vec，
		  数组中的每个元素都是 struct iovec 结构体，用于描述一个缓冲区。*/

		const int count = writable < sizeof extra_buf ? 2 : 1; // 当buffer的容量足够时，不需要读到extra_buf中。
		const ssize_t n = readv(fd, vec, count); // 最多读取128k数据
		if (n < 0) { perror("class ApplicationBuffer, function ReadFromFd, readv failed"); }
		else if (static_cast<size_t>(n) < writable) { writable_index += n; }
		else { writable_index = buffer.size(); Append(extra_buf, n - writable); }

		return n;
	}


public:
	// 返回缓冲区的总容量。
	[[nodiscard]] size_t Capacity() const { return buffer.capacity(); }
	// 收缩缓冲区，使其容量适应当前数据量。
	void Shrink() { buffer.shrink_to_fit(); }
	// 交换两个ApplicationBuffer对象的内容。
	void swap(ApplicationBuffer& rhs)
	{
		buffer.swap(rhs.buffer);
		std::swap(readable_index, rhs.readable_index);
		std::swap(writable_index, rhs.writable_index);
	}

public:
	explicit ApplicationBuffer(size_t init_size = initial_size) : buffer(reserved + init_size) { }
};