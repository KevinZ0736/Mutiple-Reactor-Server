# CMakeList.txt: CMakeProject1 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required(VERSION 3.0)

project(test)

include_directories(${HEAD_PATH})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 17)

# 将源代码添加到此项目的可执行文件。
add_executable (test_buffer "Test_Buffer.cpp" "Test_Buffer.h")

add_executable(test_thread "Test_Thread.cpp")

# 指定可执行程序要链接的动态库名字
target_link_libraries(test_thread pthread)

set(CMAKE_BUILD_TYPE RELEASE)

# TODO: 如有需要，请添加测试并安装目标。
