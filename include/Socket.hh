#pragma once

#include "InetAddress.hh"
#include "base/Macros.hh"

class Socket {
  using fd_t = int;

public:
  DISALLOW_COPY(Socket);

  Socket();
  explicit Socket(int fd);
  ~Socket();

  [[nodiscard]] int fd() const { return fd_; }

  void bindAddress(const InetAddress& localaddr) const;
  void listen() const;

  int accept(InetAddress* peeraddr) const;

  void setReuseAddr(bool on) const;

  void setReusePort(bool on) const;

  void setKeepAlive(bool on) const;

  void setTcpNoDelay(bool on) const;

private:
  const fd_t fd_;
};