#pragma once

#include "EventLoopBase.hh"

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

class EpollEventLoop : public EventLoopBase {
 public:
  using Functor = std::function<void()>;

  EpollEventLoop();
  ~EpollEventLoop() override;

  void Loop() override;
  void Quit() override;

  void RunInLoop(Functor cb) override;
  void QueueInLoop(Functor cb) override;

  void UpdateChannel(Channel* channel) override;
  void RemoveChannel(Channel* channel) override;
  bool HasChannel(Channel* channel) const override;

  [[nodiscard]] bool IsInEventLoopThread() const override { return tid_ == CurrentThread::tid(); }

 private:
  void Wakeup() const;
  void HandleRead() const;
  void DoPendingFunctors();
  static constexpr int kPollTimeMs = 10000;
  using ChannelList = std::vector<Channel*>;

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

  DISALLOW_COPY_AND_MOVE(EpollEventLoop);
};