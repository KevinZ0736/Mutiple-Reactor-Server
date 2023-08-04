// trivial
// Created by kiki on 2022/3/3.21:29
#pragma once

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
using std::move,  std::function;
using std::thread, std::mutex, std::lock_guard, std::unique_lock, std::condition_variable;
using std::atomic;
using namespace std::this_thread;