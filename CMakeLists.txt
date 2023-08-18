# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.0)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("WebServer-H")

# 测试程序生成的路径
set(TEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# 头文件目录
set(HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(TCP_HEAD ${CMAKE_CURRENT_SOURCE_DIR}/TcpServer)
set(DB_HEAD ${CMAKE_CURRENT_SOURCE_DIR}/DB)

# 静态库的名字
set(CALC_LIB calc)
set(SORT_LIB sort)

# 包含子项目。
add_subdirectory (test)
