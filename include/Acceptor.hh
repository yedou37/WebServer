#pragma once
#include "Channel.hh"
#include "EventLoop.hh"
#include "InetAddress.hh"
#include "Socket.hh"
class EventLoopBase;
class InetAddress;
class Acceptor {
public:
  using NewConnectionCallback = std::function<void(fd_t socket_fd, const InetAddress &peerAddr)>;
  Acceptor(EventLoopBase *loop, const InetAddress &listenAddr, bool reusePort = false);
  ~Acceptor();
  void SetNewConnectionCallback(const NewConnectionCallback &cb) { newConnectionCallback_ = cb; }
  [[nodiscard]] bool Listenning() const { return listenning_; }
  void Listen();

private:
  void HandleRead();
  [[maybe_unused]] EventLoopBase *loop_;
  Socket acceptSocket_;

  Channel acceptChannel_;
  bool listenning_;
  NewConnectionCallback newConnectionCallback_;
  int idleFd_;
};