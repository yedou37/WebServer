#include <iostream>

#include "Buffer.hh"
#include "EventLoop.hh"
#include "EventLoopFactory.hh"
#include "InetAddress.hh"
#include "TCPconnection.hh"
#include "TCPserver.hh"

// 用户定义的：连接建立/断开回调
void onConnection(const TCPConnectionPtr& conn) {
  if (conn->connected()) {
    std::cout << "Connection UP: " << conn->peerAddress().ToIpPort() << '\n';
  } else {
    std::cout << "Connection DOWN: " << conn->peerAddress().ToIpPort() << '\n';
  }
}

// 用户定义的：消息回调
void onMessage(const TCPConnectionPtr& conn, Buffer* buf, Timestamp time) {
  std::string msg = buf->retrieveAllAsString();
  std::cout << "Received " << msg.size() << " bytes: " << msg << '\n';

  // 业务逻辑：回显
  conn->Send(msg);

  // 业务逻辑：如果收到 "quit" 就断开
  if (msg == "quit\n" || msg == "quit\r\n") {
    conn->Shutdown();
  }
}

int main() {
  auto loop = EventLoopFactory::Create(EventLoopType::EPOLL);
  InetAddress addr("0.0.0.0", 8000);  // NOLINT 监听 8000 端口
  TCPServer server(loop.get(), addr, "MyEchoServer");

  // 注册用户回调
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);

  // 启动
  server.start();
  std::cout << "Server started on port 8000..." << '\n';

  // 进入事件循环
  loop->Loop();

  return 0;
}