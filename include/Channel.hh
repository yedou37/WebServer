#pragma once
#include <sys/epoll.h>

#include <functional>
#include <memory>

#include "base/Macros.hh"

class EventLoop;
class Channel {
public:
  using EventCallback = std::function<void()>;
  Channel(EventLoop *loop, fd_t fd);
  ~Channel();
  DISALLOW_COPY(Channel);
  void EnableRead() {
    events_ |= kReadEvent;
    Update();
  }
  void DisableRead() {
    events_ &= ~kReadEvent;
    Update();
  }
  void EnableWrite() {
    events_ |= kWriteEvent;
    Update();
  }
  void DisableWrite() {
    events_ &= ~kWriteEvent;
    Update();
  }
  void DisableAll() {
    events_ = kNoneEvent;
    Update();
  }
  void Tie(const std::shared_ptr<void> &obj);
  void SetWriteCallback(const EventCallback &callback) { writeCallback_ = callback; }
  void SetReadCallback(const EventCallback &callback) { readCallback_ = callback; }
  void SetCloseCallback(const EventCallback &callback) { closeCallback_ = callback; }
  void SetErrorCallback(const EventCallback &callback) { errorCallback_ = callback; }
  void SetIndex(const int &index) { index_ = index; }
  [[nodiscard]] bool IsNoneEvent() const { return revents_ == kNoneEvent; }
  [[nodiscard]] fd_t fd() const { return fd_; }
  [[nodiscard]] int index() const { return index_; }
  [[nodiscard]] uint32_t GetEvents() const { return events_; }
  void SetRevents(const uint32_t &revents) { revents_ = revents; }

private:
  void HandleEventWithGuard();
  void HandleEvent();
  void Update();  // call eventloop to update this channel
  EventLoop *loop_;
  fd_t fd_;
  static constexpr uint32_t kNoneEvent{0};
  static constexpr uint32_t kReadEvent{EPOLLIN | EPOLLPRI | EPOLLRDHUP};
  static constexpr uint32_t kWriteEvent{EPOLLOUT};
  static constexpr uint32_t kErrorEvent{EPOLLERR};
  static constexpr int kNew{0};
  static constexpr int kAdded{1};
  static constexpr int kDeleted{2};
  uint32_t events_{kNoneEvent};
  uint32_t revents_{kNoneEvent};
  int index_{-1};

  std::weak_ptr<void> tie_;
  bool tied_{false};
  EventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};