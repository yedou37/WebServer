cmake_minimum_required(VERSION 3.10)
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
project(Janus)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 禁用编译器特定的扩展 (如 gnu++20)，强制使用标准 ISO C++
set(CMAKE_CXX_EXTENSIONS OFF) 

option(USE_IO_URING "Enable io_uring support" ON)



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
add_executable(Janus ${SOURCES})

# Socket test executable
add_executable(SocketTest tests/SocketTest.cpp src/net/Socket.cpp src/net/InetAddress.cpp)
target_include_directories(SocketTest PRIVATE src/net)

# EventLoop test executable
add_executable(EventLoopTest tests/EventLoopTest.cpp 
                                 src/net/EpollEventLoop.cpp
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
                             src/net/EpollEventLoop.cpp
                             src/net/EventLoopFactory.cpp
                             src/net/IOUringEventLoop.cpp
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
                             src/net/IOUringEventLoop.cpp
                              src/net/InetAddress.cpp
                              src/net/EpollEventLoop.cpp
                              src/net/EventLoopThread.cpp
                              src/net/EventLoopFactory.cpp
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
                              src/net/EpollEventLoop.cpp
                              src/net/EventLoopThread.cpp
                              src/net/EventLoopThreadPool.cpp
                              src/net/EventLoopFactory.cpp
                              src/net/Channel.cpp
                             src/net/IOUringEventLoop.cpp
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
                             src/net/IOUringEventLoop.cpp
                                     src/net/EpollEventLoop.cpp
                                     src/net/EventLoopThread.cpp
                                     src/net/EventLoopThreadPool.cpp
                                     src/net/Channel.cpp
                                     src/net/Epoll.cpp
                                     src/base/Timestamp.cpp
                                     src/base/CurrentThread.cpp
                                     src/base/Buffer.cpp
                                     src/net/EventLoopFactory.cpp
                                     src/http/HttpContext.cpp
                                     src/http/HttpRequest.cpp
                                     src/http/HttpResponse.cpp
                                     src/http/HttpServer.cpp)
target_include_directories(HttpServerTest_parser PRIVATE src/net src/base src/http)

target_link_libraries(Janus pthread)

target_link_libraries(Janus uring) 


target_link_libraries(SocketTest pthread)
target_link_libraries(SocketTest uring) 
target_link_libraries(EventLoopTest pthread)
target_link_libraries(EventLoopTest uring)
target_link_libraries(AcceptorTest pthread)
target_link_libraries(AcceptorTest uring)
target_link_libraries(TCPServerTest pthread)
target_link_libraries(TCPServerTest uring)
target_link_libraries(HttpServerTest pthread)
target_link_libraries(HttpServerTest uring)
target_link_libraries(HttpServerTest_parser pthread)
target_link_libraries(HttpServerTest_parser uring)