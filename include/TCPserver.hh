#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "Acceptor.hh"
#include "EventLoop.hh"
#include "EventLoopThreadPool.hh"
#include "InetAddress.hh"
#include "TCPconnection.hh"
#include "base/Callbacks.hh"
#include "base/Macros.hh"

class TCPServer {
public:
  TCPServer(EventLoop *loop, const InetAddress &listenAddr, std::string nameArg);
  ~TCPServer();
  DISALLOW_COPY_AND_MOVE(TCPServer)
  // 调用 Acceptor::listen
  void start();

  // 设置用户层的回调函数
  void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
  void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
  void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

  // 设置线程池数量
  void setThreadNum(int numThreads);

private:
  // 当 Acceptor 接收到新连接时调用此函数
  void newConnection(int sockfd, const InetAddress &peerAddr);

  // 当 TCPConnection 关闭时调用此函数
  void removeConnection(const TCPConnectionPtr &conn);
  void removeConnectionInLoop(const TCPConnectionPtr &conn);

  using ConnectionMap = std::map<std::string, TCPConnectionPtr>;

  EventLoop *loop_;  // baseLoop
  const std::string ipPort_;
  const std::string name_;

  std::unique_ptr<Acceptor> acceptor_;  // 内部持有 Acceptor

  ConnectionMap connections_;  // 保存所有存活的连接

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;

  std::atomic_int started_;
  int nextConnId_;
  std::shared_ptr<EventLoopThreadPool> threadPool_;
};