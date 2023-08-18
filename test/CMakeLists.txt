# CMakeList.txt: CMakeProject1 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required(VERSION 3.16)

project(test)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED  True)
set(CMAKE_BUILD_TYPE DEBUG)

include_directories(${HEAD_PATH})
include_directories(${TCP_HEAD})
include_directories(${DB_HEAD})

# 创建静态库
add_library(tcp_server
        ../TcpServer/TimerContainer.cpp
        ../TcpServer/EventRegister.cpp
        ../TcpServer/EpollPoller.cpp
        ../TcpServer/Acceptor.cpp
        ../TcpServer/Reactor.cpp
        ../TcpServer/IOThreadPool.cpp
        ../TcpServer/TcpConnection.cpp
        ../TcpServer/TcpServer.cpp)

add_library(DBTool ../include/DB/ConnectionPool.cpp
                   ../include/DB/GetCols.cpp
                   ../include/DB/_mysql.cpp
                   ../include/DB/_public.cpp)

target_link_libraries(tcp_server pthread)

add_library(file_operation ../include/File_Operation.cpp)

target_link_libraries(tcp_server file_operation)

# 将源代码添加到此项目的可执行文件。
add_executable (test_buffer "Test_Buffer.cpp" "Test_Buffer.h")

add_executable(test_thread "Test_Thread.cpp")

add_executable(EchoServer EchoServer.cpp)
target_link_libraries(EchoServer tcp_server)

add_executable(HttpServer HttpServer.cpp)
target_link_libraries(HttpServer tcp_server DBTool pthread)

# 指定可执行程序要链接的动态库名字
target_link_libraries(test_thread pthread)

set(CMAKE_BUILD_TYPE RELEASE)

# TODO: 如有需要，请添加测试并安装目标。
