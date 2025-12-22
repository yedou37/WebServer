#pragma once

#include <liburing.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "EventLoopBase.hh"
#include "base/CurrentThread.hh"
#include "base/Macros.hh"
#include "base/Timestamp.hh"

class Channel;

class IOUringEventLoop : public EventLoopBase {
public:
  using Functor = std::function<void()>;

  using ChannelList = std::vector<Channel*>;
  IOUringEventLoop();
  ~IOUringEventLoop() override;

  void Loop() override;
  void Quit() override;

  void RunInLoop(Functor cb) override;
  void QueueInLoop(Functor cb) override;

  void UpdateChannel(Channel* channel) override;
  void RemoveChannel(Channel* channel) override;
  bool HasChannel(Channel* channel) const override;

  [[nodiscard]] bool IsInEventLoopThread() const override { return tid_ == CurrentThread::tid(); }

  struct io_uring* GetRing() { return &ring_; }
  struct io_uring_sqe* GetSqe();

private:
  void Wakeup() const;
  void HandleRead() const;
  void DoPendingFunctors();

  std::atomic_bool looping_{false};
  std::atomic_bool quit_{false};
  const pid_t tid_;

  int wakeupFd_;
  std::unique_ptr<Channel> wakeupChannel_;

  std::mutex mutex_;
  std::vector<Functor> pendingFunctors_;
  bool callingPendingFunctors_{false};
  std::unordered_map<int, Channel*> channels_;
  struct io_uring ring_;

  DISALLOW_COPY_AND_MOVE(IOUringEventLoop);
};