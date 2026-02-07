#include <csignal>
#include <iostream>
#include <string>

#include "EventLoop.hh"
#include "InetAddress.hh"
#include "base/Logger.hh"
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
  Logger::instance().initAsyncLog("test.log");
  LOG_INFO("init async log");
  EventLoop loop;
  InetAddress addr("0.0.0.0", 8080);  // NOLINT
  ::signal(SIGPIPE, SIG_IGN);
  // 创建 Server
  HttpServer server(&loop, addr, "MyHttpServer");
  LOG_INFO("Server listening: ip: %s, port: %d", addr.ToIp().c_str(), addr.ToPort());
  // 1. 设置回调
  server.setHttpCallback(onRequest);

  // 2. 设置线程数
  server.setThreadNum(8);  // NOLINT
  LOG_INFO("Server started. %d threads started", 8);
  // 3. 启动
  server.start();

  loop.Loop();
  return 0;
}