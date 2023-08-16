// trivial_test
// Created by kiki on 2021/12/2.11:29
#pragma once
#include "./head/cpp_thread.h"
#include "./head/cpp_container.h"
#include <iostream>
using std::clog; // clog与cerr都是输出到标准错误，clog带缓冲，cerr不带缓冲。

static mutex global_log_mutex; // 使用全局锁的控制台日志，对性能影响较大，仅作debug用。

static const unordered_map<const char*, const char*> color = // constexpr只能修饰基本类型和指针。
{
	{"black",      "\033[30m"},
	{"red",        "\033[31m"},
	{"green",      "\033[32m"},
	{"yellow",     "\033[33m"},
	{"blue",       "\033[34m"},
	{"purple",     "\033[35m"},
	{"dark_green", "\033[36m"},
	{"white",      "\033[37m"}
};

/*
* c++17折叠表达式
* (..., out_with_space(args)); 展开为out_with_space(arg1)，out_with_space(arg2)， out_with_space(arg3) ......
* ...后面的逗号是一个二元运算符
*/
template <typename First, typename... Args> // 可以使用万能引用和完美转发，但没这个必要。
void
consile_print
(
	string_view colour,
	string_view file, int line, string_view function, std::thread::id tid,
	const First& first, const Args&... args
)
{
	lock_guard<mutex> guard(global_log_mutex);

	clog << colour;

	clog << file << ":" << line << ", function " << function << ", thread " << tid << "---";

	clog << first;
	auto out_with_space =
		[](const auto& arg)
	{
		clog << ' ' << arg;
	};
	(..., out_with_space(args));

	clog << '\n';
}


#define log(...) consile_print(color.at("blue"), __FILE__, __LINE__, __FUNCTION__, get_id(), ##__VA_ARGS__);
#define trace(...) consile_print(color.at("green"), __FILE__, __LINE__, __FUNCTION__, get_id(), ##__VA_ARGS__);
#define debug(...) consile_print(color.at("dark_green"), __FILE__, __LINE__, __FUNCTION__, get_id(), ##__VA_ARGS__);
#define info(...) consile_print(color.at("red"), __FILE__, __LINE__, __FUNCTION__, get_id(), ##__VA_ARGS__);

#define err(...) consile_print(color.at("white"), __FILE__, __LINE__, __FUNCTION__, get_id(), ##__VA_ARGS__);
#define fatal(...) \
do                 \
{                  \
	consile_print(color.at("black"), __FILE__, __LINE__, __FUNCTION__, get_id(), ##__VA_ARGS__); \
	std::abort(); \
} \
while (0);         \

