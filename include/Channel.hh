#pragma once

#include <sys/epoll.h>

#include <functional>
#include <memory>

#include "base/Macros.hh"  // 假设你有 DISALLOW_COPY
#include "base/Timestamp.hh"

class EventLoop;  // 前置声明，避免循环包含

class Channel {
public:
  using EventCallback = std::function<void()>;
  using ReadEventCallback = std::function<void(Timestamp)>;  // 读回调包含时间戳

  Channel(EventLoop *loop, int fd);
  ~Channel();

  DISALLOW_COPY(Channel);

  // --- 核心事件处理 ---
  // receiveTime 是 Poller 返回事件的时间点
  void HandleEvent(Timestamp receiveTime);

  // --- 事件注册/注销 ---
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

  // --- 状态判断 ---
  [[nodiscard]] bool IsNoneEvent() const { return events_ == kNoneEvent; }
  [[nodiscard]] bool IsWriting() const { return (events_ & kWriteEvent) != 0U; }
  [[nodiscard]] bool IsReading() const { return (events_ & kReadEvent) != 0U; }

  // --- Getters & Setters ---
  [[nodiscard]] int Getfd() const { return fd_; }
  [[nodiscard]] int GetIndex() const { return index_; }
  void SetIndex(int idx) { index_ = idx; }  // 供 Poller 使用

  [[nodiscard]] uint32_t GetEvents() const { return events_; }
  void SetRevents(uint32_t revt) { revents_ = revt; }  // 供 Poller 使用

  EventLoop *ownerLoop() { return loop_; }

  // --- 绑定生命周期 ---
  // 防止 Channel 执行回调时，对象(如TcpConnection)已经被销毁
  void Tie(const std::shared_ptr<void> &obj);

  // --- 回调设置 ---
  void SetReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
  void SetWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
  void SetCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
  void SetErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

private:
  void Update();
  void HandleEventWithGuard(Timestamp receiveTime);

  // EPOLL 事件常量
  static constexpr uint32_t kNoneEvent = 0;
  static constexpr uint32_t kReadEvent = EPOLLIN | EPOLLPRI;
  static constexpr uint32_t kWriteEvent = EPOLLOUT;

  EventLoop *loop_;
  const fd_t fd_;

  uint32_t events_{0};   // 用户关心的事件
  uint32_t revents_{0};  // Poller 返回的实际发生事件
  int index_{-1};        // Used by Poller (kNew, kAdded, kDeleted)

  std::weak_ptr<void> tie_;
  bool tied_{false};

  // 回调函数
  ReadEventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback closeCallback_;
  EventCallback errorCallback_;
};