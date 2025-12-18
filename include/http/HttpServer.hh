#pragma once
#include "TCPserver.hh"
#include "base/Callbacks.hh"
#include "http/HttpContext.hh"
#include "http/HttpRequest.hh"
#include "http/HttpResponse.hh"

class HttpServer {
public:
  using HttpCallback = std::function<void(const HttpRequest&, HttpResponse*)>;
  HttpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name);

  ~HttpServer();

  [[nodiscard]] EventLoop* getLoop() const { return server_.getLoop(); }

  void setHttpCallback(const HttpCallback& cb) { httpCallback_ = cb; }

  void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

  void start();

private:
  void onConnection(const TCPConnectionPtr& conn);
  void onMessage(const TCPConnectionPtr& conn, Buffer* buf, Timestamp time);
  void onRequest(const TCPConnectionPtr& conn, const HttpRequest& req);
  TCPServer server_;
  HttpCallback httpCallback_;
};