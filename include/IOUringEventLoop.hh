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

#ifdef USE_IO_URING
#include <liburing.h>
#endif

class Channel;

class IOUringEventLoop : public EventLoopBase {
 public:
  using Functor = std::function<void()>;

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

#ifdef USE_IO_URING
  struct io_uring* GetRing() { return &ring_; }
#endif

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

#ifdef USE_IO_URING
  struct io_uring ring_;
#endif

  DISALLOW_COPY_AND_MOVE(IOUringEventLoop);
};