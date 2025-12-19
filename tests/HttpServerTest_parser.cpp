#include <signal.h>

#include <iostream>
#include <string>

#include "EventLoop.hh"
#include "InetAddress.hh"
#include "http/HttpRequest.hh"
#include "http/HttpResponse.hh"
#include "http/HttpServer.hh"

// 业务逻辑回调函数
void onRequest(const HttpRequest& req, HttpResponse* resp) {
  if (req.path() == "/") {
    resp->SetStatusCode(HttpResponse::HttpStatusCode::k200Ok);
    resp->SetStatusMessage("OK");
    resp->SetContentType("text/html");
    resp->SetBody("<h1>Hello, High Performance C++ Server!</h1>");
  } else if (req.path() == "/hello") {
    resp->SetStatusCode(HttpResponse::HttpStatusCode::k200Ok);
    resp->SetContentType("text/plain");
    resp->SetBody("Hello World");
  } else {
    resp->SetStatusCode(HttpResponse::HttpStatusCode::k404NotFound);
    resp->SetStatusMessage("Not Found");
    resp->SetBody("404 Not Found");
    resp->SetCloseConnection(true);
  }
}

int main(int argc, char* argv[]) {
  EventLoop loop;
  InetAddress addr("0.0.0.0", 8080);  // NOLINT
  ::signal(SIGPIPE, SIG_IGN);
  // 创建 Server
  HttpServer server(&loop, addr, "MyHttpServer");

  // 1. 设置回调
  server.setHttpCallback(onRequest);

  // 2. 设置线程数
  server.setThreadNum(8);  // NOLINT

  // 3. 启动
  server.start();

  std::cout << "Server is running on port 8080..." << '\n';

  loop.Loop();
  return 0;
}