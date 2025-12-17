#pragma once

#include <memory>
#include <utility>

#include "Buffer.hh"
#include "Channel.hh"
#include "EventLoop.hh"
#include "InetAddress.hh"
#include "Socket.hh"
#include "base/Callbacks.hh"
#include "base/Macros.hh"
class InetAddress;
class EventLoop;
class TCPConnection : public std::enable_shared_from_this<TCPConnection> {
public:
  TCPConnection(EventLoop *loop, std::string nameArg, fd_t sockfd, const InetAddress &local_addr,
                const InetAddress &peer_addr);

  DISALLOW_COPY(TCPConnection);
  ~TCPConnection();
  void ConnectEstablished();
  void ConnectDestroyed();
  void Send(const std::string &message);
  void Shutdown();

  EventLoop *getLoop() const { return loop_; }
  const std::string &name() const { return name_; }
  const InetAddress &localAddress() const { return local_addr_; }
  const InetAddress &peerAddress() const { return peer_addr_; }

  [[nodiscard]] bool connected() const { return state_ == State::kConnected; }
  void setConnectionCallback(const ConnectionCallback &cb) { connection_callback_ = cb; }
  void setMessageCallback(const MessageCallback &cb) { message_callback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) { write_complete_callback_ = cb; }
  void setCloseCallback(const CloseCallback &cb) { close_callback_ = cb; }

private:
  enum class State : std::uint8_t { kConnecting, kConnected, kDisconnecting, kDisconnected };
  void HandleRead();
  void HandleWrite();
  void HandleClose();
  void HandleError();
  void sendInLoop(const void *data, size_t len);
  void shutdownInLoop();

  void setState(State s) { state_.store(s); }
  EventLoop *loop_;
  const std::string name_;
  std::atomic<State> state_;
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const InetAddress local_addr_;
  const InetAddress peer_addr_;
  bool reading_;

  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;
  CloseCallback close_callback_;

  Buffer input_buffer_;
  Buffer output_buffer_;
};