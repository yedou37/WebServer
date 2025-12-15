
#include "Acceptor.hh"

#include <fcntl.h>
#include <unistd.h>

#include <cassert>

#include "base/Timestamp.hh"

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    : loop_(loop),
      acceptChannel_(loop, acceptSocket_.fd()),
      listenning_(false),
      idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReusePort(reuseport);

  acceptSocket_.bindAddress(listenAddr);

  // 注册 Channel 的读回调
  // 当 epoll 监听到 listenfd 有事件，会调用 handleRead
  acceptChannel_.SetReadCallback([this](Timestamp) { HandleRead(); });
}

Acceptor::~Acceptor() {
  acceptChannel_.DisableAll();
  acceptChannel_.Remove();
  ::close(idleFd_);
}

void Acceptor::Listen() {
  listenning_ = true;
  acceptSocket_.listen();       // 调用系统 listen
  acceptChannel_.EnableRead();  // 注册到 Epoll，开始关注可读事件
}

void Acceptor::HandleRead() {
  assert(loop_->IsInEventLoopThread());

  InetAddress peerAddr;

  int connfd = acceptSocket_.accept(&peerAddr);

  if (connfd >= 0) {
    if (newConnectionCallback_) {
      // 将新连接分发出去 (给 TcpServer)
      newConnectionCallback_(connfd, peerAddr);
    } else {
      ::close(connfd);
    }
  } else {
    // 错误处理，特别是 EMFILE (文件描述符耗尽)
    if (errno == EMFILE) {
      ::close(idleFd_);                                          // 先关闭空闲 fd，腾出一个位置
      idleFd_ = ::accept(acceptSocket_.fd(), nullptr, nullptr);  // 接受这个连接
      ::close(idleFd_);                                          // 马上把这个连接关掉
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);       // 重新把坑占上
    }
  }
}