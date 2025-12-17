#include <iostream>

#include "Buffer.hh"
#include "EventLoop.hh"
#include "InetAddress.hh"
#include "TCPconnection.hh"
#include "TCPserver.hh"

// 用户定义的：连接建立/断开回调
void onConnection(const TCPConnectionPtr& conn) {
  /*
  if (conn->connected()) {
    std::cout << "HTTP Connection UP: " << conn->peerAddress().ToIpPort() << '\n';
  } else {
    std::cout << "HTTP Connection DOWN: " << conn->peerAddress().ToIpPort() << '\n';
  }
    */
}

// 伪装成 HTTP Server 的回调
void onMessage(const TCPConnectionPtr& conn, Buffer* buf, Timestamp time) {
  // 1. 取出数据（假装我们在解析 HTTP，实际直接扔掉）
  std::string msg = buf->retrieveAllAsString();
  (void)msg;
  // 2. 构造一个最简单的 HTTP 响应
  std::string response =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n"
      "Content-Length: 11\r\n"
      "\r\n"
      "Hello World";

  // 3. 发送响应
  conn->Send(response);

  // 4. 关键：发送完立刻断开连接（短连接模式）
  // 这样 Webbench 才知道"这个请求处理完了"，才会去发下一个
  // 如果测测试长连接，就不要调用 Shutdown()
  // conn->Shutdown();
}

int main() {
  EventLoop loop;
  InetAddress addr("0.0.0.0", 8080);  // NOLINT 监听 8080 端口，这是 HTTP 服务常用端口
  TCPServer server(&loop, addr, "MyHttpServer");

  // 注册用户回调
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);

  // 启动
  server.start();
  std::cout << "HTTP Server started on port 8080..." << '\n';

  // 进入事件循环
  loop.Loop();

  return 0;
}