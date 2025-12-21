#include <unistd.h>

#include <iostream>

#include "Acceptor.hh"
#include "EventLoop.hh"
#include "EventLoopFactory.hh"
#include "InetAddress.hh"

// 模拟 TcpServer 的回调行为
void newConnection(int sockfd, const InetAddress& peerAddr) {
  std::cout << "Successfully accepted a new connection!" << '\n';
  std::cout << "  - FD: " << sockfd << '\n';
  std::cout << "  - IP: " << peerAddr.ToIp() << '\n';
  std::cout << "  - Port: " << peerAddr.ToPort() << '\n';
  Socket socket(sockfd);
  // 简单的打招呼
  socket.write("Hello from Acceptor Test!\n", 26);  // NOLINT
  // 这里不用手动close 因为已经交给socket管理了 会自动析构
  sleep(1);                                       // NOLINT
  socket.write("Bye from Acceptor Test!\n", 24);  // NOLINT
}

int main() {
  std::cout << "main(): starting" << '\n';

  InetAddress listenAddr("0.0.0.0", 8888);  // NOLINT
  auto loop = EventLoopFactory::Create(EventLoopType::EPOLL);

  Acceptor acceptor(loop.get(), listenAddr, true);

  // 设置回调
  acceptor.SetNewConnectionCallback(newConnection);

  // 开始监听
  acceptor.Listen();
  std::cout << "Acceptor is listening on port 8888..." << '\n';

  // 启动循环
  loop->Loop();

  return 0;
}