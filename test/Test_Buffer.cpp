// CMakeProject1.cpp: 定义应用程序的入口点。
//

#include "Test_Buffer.h"
#include "ApplicationBuffer.hpp"

using namespace std;

int main()
{
	ApplicationBuffer buffer(100);
	cout << "init : =============================================" << endl;
	cout << "buffer.ReadableBytes() " << buffer.ReadableBytes() << " should be 0" << endl;
	cout << "buffer.WritableBytes() " << buffer.WritableBytes() << " should be 100" << endl;
	cout << "buffer.Capacity() " << buffer.Capacity() << " should be 108" << endl;
	cout << "====================================" << endl;
	buffer.Append("hello world", 11);
	buffer.PrePend("1234567 ", 8);
	cout << buffer.StringView() << endl; // 1234567 hello world
	cout << buffer.ReadableBytes() << endl; // 19
	cout << buffer.WritableBytes() << endl; // 89
	cout << buffer.RetrieveAsString() << endl; // 1234567 hello world
	cout << buffer.ReadableBytes() << endl; // 0
	cout << buffer.WritableBytes() << endl; // 100
	cout << "====================================" << endl;
	string str1(100, '\r');
	buffer.Append(str1.data(), str1.size());
	string str2(100, '\n');
	buffer.Append(str2.data(), str2.size());
	cout << buffer.ReadableBytes() << endl; // 200
	cout << buffer.WritableBytes() << endl; // 0
	cout << "buffer.Capacity() " << buffer.Capacity() << endl; // buffer.Capacity() 216
	cout << buffer.FindCRLF() - buffer.readable_ptr() << endl; // 99
	cout << buffer.FindEOL() - buffer.readable_ptr() << endl; // 100
	cout << "====================================" << endl;
	buffer.Append("aaaaaaaa", 8);
	cout << buffer.ReadableBytes() << endl; // 208
	cout << buffer.WritableBytes() << endl; // 0
	cout << "buffer.Capacity() " << buffer.Capacity() << endl; // buffer.Capacity() 216
	cout << "====================================" << endl;
	buffer.Append("b", 1);
	cout << buffer.ReadableBytes() << endl; // 209
	cout << buffer.WritableBytes() << endl; // 0
	cout << "buffer.Capacity() " << buffer.Capacity() << endl; // buffer.Capacity() 432
}