#include "TCPserver.hh"

#include <strings.h>

#include <cstring>
#include <functional>
#include <utility>

#include "EventLoopThread.hh"
#include "EventLoopThreadPool.hh"
#include "Socket.hh"

TCPServer::TCPServer(EventLoop *loop, const InetAddress &listenAddr, std::string nameArg)
    : loop_(loop),
      ipPort_(listenAddr.ToIpPort()),
      name_(std::move(nameArg)),
      acceptor_(std::make_unique<Acceptor>(loop, listenAddr, true)),
      started_(0),
      nextConnId_(1),
      threadPool_(std::make_shared<EventLoopThreadPool>(loop, name_)) {
  acceptor_->SetNewConnectionCallback(
      [this](fd_t socket_fd, const InetAddress &peerAddr) { newConnection(socket_fd, peerAddr); });
}

TCPServer::~TCPServer() {
  // 析构时，销毁所有连接
  for (auto &item : connections_) {
    TCPConnectionPtr conn(item.second);  // 局部智能指针持有，防止引用计数变为0直接销毁
    item.second.reset();                 // 释放 Map 中的控制权

    conn->getLoop()->RunInLoop([conn]() { conn->ConnectDestroyed(); });
  }
}

void TCPServer::start() {
  assert(loop_->IsInEventLoopThread() == true);
  if (started_.fetch_add(1) == 0) {
    // 启动线程池
    threadPool_->start();

    // 启动 Acceptor 监听
    loop_->RunInLoop([capture0 = acceptor_.get()] { capture0->Listen(); });
  }
}

void TCPServer::newConnection(int sockfd, const InetAddress &peerAddr) {
  assert(loop_->IsInEventLoopThread());

  // 使用线程池获取下一个EventLoop
  EventLoop *ioLoop = threadPool_->getNextLoop();
  std::array<char, 64> buf{};  // NOLINT
  snprintf(buf.data(), buf.size(), "-%s#%d", ipPort_.c_str(), nextConnId_);
  ++nextConnId_;
  std::string connName = name_ + buf.data();

  struct sockaddr_in local;
  ::memset(&local, 0, sizeof local);
  socklen_t addrlen = sizeof local;
  if (::getsockname(sockfd, reinterpret_cast<struct sockaddr *>(&local), &addrlen) < 0) {
    perror("getsockname");
  }
  InetAddress localAddr(local);

  // 5. 创建 TCPConnection 对象
  TCPConnectionPtr conn = std::make_shared<TCPConnection>(ioLoop, connName, sockfd, localAddr, peerAddr);

  // 6. 放入 Map
  connections_.emplace(connName, conn);

  // 7. 设置回调
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);

  // 设置关闭回调：当 TCPConnection 断开时，要回调 TcpServer::removeConnection
  conn->setCloseCallback([this](auto &&PH1) { removeConnection(std::forward<decltype(PH1)>(PH1)); });

  // 8. 真正建立连接
  ioLoop->RunInLoop([conn] { conn->ConnectEstablished(); });
}

void TCPServer::removeConnection(const TCPConnectionPtr &conn) {
  loop_->RunInLoop([this, conn]() { removeConnectionInLoop(conn); });
}

void TCPServer::removeConnectionInLoop(const TCPConnectionPtr &conn) {
  // 1. 从 Map 中删除
  size_t n = connections_.erase(conn->name());
  (void)n;
  assert(n == 1);

  // 2. 此时 Map 已经不持有 conn 了，但参数 shared_ptr 还在持有
  // 3. 调用 ConnectDestroyed 进行最后清理
  EventLoop *ioLoop = conn->getLoop();
  ioLoop->QueueInLoop([conn]() { conn->ConnectDestroyed(); });
}

void TCPServer::setThreadNum(int numThreads) {
  threadPool_->setThreadNum(numThreads);
}