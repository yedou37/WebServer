#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "base/CurrentThread.hh"
#include "base/Macros.hh"
#include "base/Timestamp.hh"

class Channel;
class Epoll;

class EventLoop {
public:
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();

  void Loop();
  void Quit();

  // 用户调用的入口
  // loopA->RunInLoop( [=](){ loopA->epoll_->add(fd); } )
  void RunInLoop(Functor cb);

  // 给 Channel 调用的
  void UpdateChannel(Channel *channel);
  void RemoveChannel(Channel *channel);
  bool HasChannel(Channel *channel) const;

  [[nodiscard]] bool IsInEventLoopThread() const { return tid_ == CurrentThread::tid(); }

private:
  void Wakeup() const;
  void HandleRead() const;
  void QueueInLoop(Functor cb);
  void DoPendingFunctors();
  static constexpr int kPollTimeMs = 10000;
  using ChannelList = std::vector<Channel *>;

  std::atomic_bool looping_{false};
  std::atomic_bool quit_{false};
  const pid_t tid_;

  std::unique_ptr<Epoll> epoll_;

  int wakeupFd_;
  std::unique_ptr<Channel> wakeupChannel_;

  ChannelList activeChannels_;

  std::mutex mutex_;
  std::vector<Functor> pendingFunctors_;
  bool callingPendingFunctors_{false};
};