cmake_minimum_required(VERSION 3.10)
project(Janus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 禁用编译器特定的扩展 (如 gnu++20)，强制使用标准 ISO C++
set(CMAKE_CXX_EXTENSIONS OFF) 

# 2. 设置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 3. 编译选项 
# -Wall -Wextra: 基础警告
add_compile_options(-Wall -Wextra -Werror -Wno-unused-parameter -pthread)

# 调试模式下添加调试信息，Release模式下开启 O3 优化
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
else()
    add_compile_options(-O3)
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src) 

# Main executable
file(GLOB_RECURSE SOURCES "src/*.cpp")
# 移除main.cpp以避免与HttpServerMain.cpp中的main函数冲突
list(FILTER SOURCES EXCLUDE REGEX "main\\.cpp$")
add_executable(Janus ${SOURCES})
target_link_libraries(Janus PRIVATE ${LIBRT} pthread)

# Socket test executable
add_executable(SocketTest tests/SocketTest.cpp src/net/Socket.cpp src/net/InetAddress.cpp)
target_include_directories(SocketTest PRIVATE src/net)

# EventLoop test executable
add_executable(EventLoopTest tests/EventLoopTest.cpp 
                                 src/net/EventLoop.cpp 
                                 src/net/Channel.cpp 
                                 src/net/Epoll.cpp
                                 src/base/Timestamp.cpp
                                 src/base/CurrentThread.cpp)
target_include_directories(EventLoopTest PRIVATE src/net src/base)

# Acceptor test executable
add_executable(AcceptorTest tests/AcceptorTest.cpp
                             src/net/Acceptor.cpp
                             src/net/Socket.cpp
                             src/net/InetAddress.cpp
                             src/net/EventLoop.cpp
                             src/net/Channel.cpp
                             src/net/Epoll.cpp
                             src/base/Timestamp.cpp
                             src/base/CurrentThread.cpp)
target_include_directories(AcceptorTest PRIVATE src/net src/base)

# TCPServer test executable
add_executable(TCPServerTest tests/TCPServerTest.cpp
                              src/net/TCPserver.cpp
                              src/net/TCPconnection.cpp
                              src/net/Acceptor.cpp
                              src/net/Socket.cpp
                              src/net/InetAddress.cpp
                              src/net/EventLoop.cpp
                              src/net/EventLoopThread.cpp
                              src/net/EventLoopThreadPool.cpp
                              src/net/Channel.cpp
                              src/net/Epoll.cpp
                              src/base/Timestamp.cpp
                              src/base/CurrentThread.cpp
                              src/base/Buffer.cpp)
target_include_directories(TCPServerTest PRIVATE src/net src/base)

# HTTP Server test executable
add_executable(HttpServerTest tests/HttpServerTest_echo.cpp
                              src/net/TCPserver.cpp
                              src/net/TCPconnection.cpp
                              src/net/Acceptor.cpp
                              src/net/Socket.cpp
                              src/net/InetAddress.cpp
                              src/net/EventLoop.cpp
                              src/net/EventLoopThread.cpp
                              src/net/EventLoopThreadPool.cpp
                              src/net/Channel.cpp
                              src/net/Epoll.cpp
                              src/base/Timestamp.cpp
                              src/base/CurrentThread.cpp
                              src/base/Buffer.cpp)
target_include_directories(HttpServerTest PRIVATE src/net src/base)

# HTTP Server parser test executable
add_executable(HttpServerTest_parser tests/HttpServerTest_parser.cpp
                                     src/net/TCPserver.cpp
                                     src/net/TCPconnection.cpp
                                     src/net/Acceptor.cpp
                                     src/net/Socket.cpp
                                     src/net/InetAddress.cpp
                                     src/net/EventLoop.cpp
                                     src/net/EventLoopThread.cpp
                                     src/net/EventLoopThreadPool.cpp
                                     src/net/Channel.cpp
                                     src/net/Epoll.cpp
                                     src/base/Timestamp.cpp
                                     src/base/CurrentThread.cpp
                                     src/base/Buffer.cpp
                                     src/http/HttpContext.cpp
                                     src/http/HttpRequest.cpp
                                     src/http/HttpResponse.cpp
                                     src/http/HttpServer.cpp)
target_include_directories(HttpServerTest_parser PRIVATE src/net src/base src/http)

# 查找系统库
find_library(LIBRT rt)

# 添加可执行文件 (原始的套接字测试程序)
# add_executable(Janus ${SRC_LIST})
# target_link_libraries(Janus PRIVATE ${LIBRT} pthread)

# 添加新的HTTP服务器可执行文件
add_executable(HttpServerMain 
    src/http/HttpServerMain.cpp
    src/http/HttpRequest.cpp
    src/http/HttpResponse.cpp
    src/http/HttpServer.cpp
    src/http/HttpContext.cpp
    src/http/HomeworkHandler.cpp
    src/net/Acceptor.cpp
    src/net/Channel.cpp
    src/net/Epoll.cpp
    src/net/EventLoop.cpp
    src/net/EventLoopThread.cpp
    src/net/EventLoopThreadPool.cpp
    src/net/InetAddress.cpp
    src/net/Socket.cpp
    src/net/TCPconnection.cpp
    src/net/TCPserver.cpp
    src/base/Buffer.cpp
    src/base/CurrentThread.cpp
    src/base/Timestamp.cpp
)

# 链接到必要的库
target_link_libraries(HttpServerMain PRIVATE ${LIBRT} pthread stdc++fs)

# 设置HTTP服务器的包含目录
target_include_directories(HttpServerMain PRIVATE ${PROJECT_SOURCE_DIR}/include)

# 设置编译选项
target_compile_options(HttpServerMain PRIVATE
    -Wall
    -Wextra
    -Werror
    -pedantic-errors
)

# 设置C++标准
set_property(TARGET HttpServerMain PROPERTY CXX_STANDARD 20)
set_property(TARGET HttpServerMain PROPERTY CXX_STANDARD_REQUIRED ON)
