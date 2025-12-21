#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "base/CurrentThread.hh"
#include "base/Timestamp.hh"
#include "base/Macros.hh"

class Channel;

class EventLoopBase {
 public:
  using Functor = std::function<void()>;

  EventLoopBase() = default;
  virtual ~EventLoopBase() = default;

  virtual void Loop() = 0;
  virtual void Quit() = 0;

  virtual void RunInLoop(Functor cb) = 0;
  virtual void QueueInLoop(Functor cb) = 0;

  virtual void UpdateChannel(Channel* channel) = 0;
  virtual void RemoveChannel(Channel* channel) = 0;
  virtual bool HasChannel(Channel* channel) const = 0;

  [[nodiscard]] virtual bool IsInEventLoopThread() const = 0;

 protected:
  DISALLOW_COPY_AND_MOVE(EventLoopBase);
};