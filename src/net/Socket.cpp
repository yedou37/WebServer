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
fd_t Socket::accept(InetAddress* peeraddr) const {
  InetAddress::sin_t addr;
  socklen_t len = sizeof(addr);
  memset(&addr, 0, sizeof(addr));

  fd_t connfd = ::accept4(fd_, reinterpret_cast<sockaddr*>(&addr), &len, SOCK_NONBLOCK | SOCK_CLOEXEC);

  if (connfd >= 0) {
    if (peeraddr != nullptr) {
      *peeraddr = InetAddress(addr);
    }
  }

  return connfd;
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

void Socket::shutdownWrite() const {
  ::shutdown(fd_, SHUT_WR);
}