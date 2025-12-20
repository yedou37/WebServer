#include <unistd.h>

#include <iostream>

#include "EventLoop.hh"
#include "InetAddress.hh"
#include "http/HomeworkHandler.h"
#include "http/HttpServer.hh"

void httpCallback(const HttpRequest& req, HttpResponse* resp) {
  HomeworkHandler::handle(req, resp);
}

int main() {
  // 使用学号的后4位作为服务器的监听端口: 1885
  constexpr InetAddress::port_t port = 1885;
  InetAddress listenAddr("0.0.0.0", port);

  std::cout << "Starting HTTP server on port " << port << std::endl;

  EventLoop loop;
  HttpServer server(&loop, listenAddr, "HomeworkServer");

  // 注册我们的处理函数
  server.setHttpCallback(httpCallback);

  // 启动服务器
  server.start();
  loop.Loop();

  return 0;
}