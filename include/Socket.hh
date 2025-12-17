#pragma once

#include "InetAddress.hh"
#include "base/Macros.hh"

class Socket {
public:
  DISALLOW_COPY(Socket);

  Socket();
  explicit Socket(int fd);
  Socket(Socket&& rhs) noexcept;
  Socket& operator=(Socket&& rhs) noexcept;
  ~Socket();
  ssize_t read(void* buf, size_t count) const;

  ssize_t write(const void* buf, size_t count) const;
  [[nodiscard]] int fd() const { return fd_; }

  void bindAddress(const InetAddress& localaddr) const;
  void listen() const;

  fd_t accept(InetAddress* peeraddr) const;

  void setReuseAddr(bool on) const;

  void setReusePort(bool on) const;

  void setKeepAlive(bool on) const;

  void setTcpNoDelay(bool on) const;
  void shutdownWrite() const;

private:
  void swap(Socket& other) noexcept;
  fd_t fd_;
};