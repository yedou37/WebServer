#include "Socket.hh"

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

#include "InetAddress.hh"
void Socket::swap(Socket& other) noexcept {
  std::swap(fd_, other.fd_);
}
Socket::Socket() : fd_(::socket(AF_INET, SOCK_STREAM, 0)) {
  if (fd_ == -1) {
    perror("Socket::Socket(): socket() failed");
    throw std::runtime_error("Socket::Socket(): socket() failed");
  }
}

Socket::Socket(int fd) : fd_(fd) {
  if (fd_ < 0) {
    perror("Socket create error");
    throw std::runtime_error("Socket create error");
  }
}
Socket::Socket(Socket&& rhs) noexcept : fd_(-1) {
  this->swap(rhs);
}
Socket& Socket::operator=(Socket&& rhs) noexcept {
  if (this != &rhs) {
    Socket tmp(std::move(rhs));
    this->swap(tmp);
  }
  return *this;
}

Socket::~Socket() {
  if (fd_ != -1) {
    ::close(fd_);
  }
}

ssize_t Socket::read(void* buf, size_t count) const {
  return ::read(fd_, buf, count);
}

ssize_t Socket::write(const void* buf, size_t count) const {
  return ::write(fd_, buf, count);
}
void Socket::bindAddress(const InetAddress& localaddr) const {
  auto ret = ::bind(fd_, localaddr.GetSocketAddr(), localaddr.GetSocketAddrLen());
  if (ret != 0) {
    perror("Socket::bindAddress error");
    throw std::runtime_error("Socket::bindAddress error");
  }
}

void Socket::listen() const {
  auto ret = ::listen(fd_, SOMAXCONN);
  if (ret != 0) {
    perror("Socket::listen error");
    throw std::runtime_error("Socket::listen error");
  }
}

Socket Socket::accept(InetAddress* peeraddr) const {
  InetAddress::sin_t addr;
  socklen_t len = sizeof(addr);
  memset(&addr, 0, sizeof(addr));

  int connfd = ::accept(fd_, reinterpret_cast<sockaddr*>(&addr), &len);

  if (connfd >= 0) {
    // 如果成功，利用 InetAddress 的构造函数，把 sock_in 转换成对象
    // 这里利用了赋值操作符，将 peeraddr 指向的内容更新
    if (peeraddr != nullptr) {
      *peeraddr = InetAddress(addr);
    }
  } else {
    // TODO(yedou): 在非阻塞模式下，需要处理 EAGAIN 和 EWOULDBLOCK，不能视为错误
    perror("Socket::accept error");
  }

  return Socket{connfd};
}

void Socket::setReuseAddr(bool on) const {
  int optval = on ? 1 : 0;
  ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}

void Socket::setReusePort(bool on) const {
  int optval = on ? 1 : 0;
  ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

void Socket::setKeepAlive(bool on) const {
  int optval = on ? 1 : 0;
  ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}

void Socket::setTcpNoDelay(bool on) const {
  int optval = on ? 1 : 0;
  // 禁用 Nagle 算法
  ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}