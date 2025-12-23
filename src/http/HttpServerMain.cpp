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
  constexpr InetAddress::port_t port = 31885;
  InetAddress listenAddr("0.0.0.0", port);

  std::cout << "Starting HTTP server on port " << port << '\n';

  EventLoop loop;
  HttpServer server(&loop, listenAddr, "HomeworkServer");

  server.setHttpCallback(httpCallback);

  server.start();
  loop.Loop();

  return 0;
}