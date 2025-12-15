#include "Channel.hh"

#include <sys/epoll.h>

#include <cassert>

#include "EventLoop.hh"  // 必须包含，因为要调用 loop_->UpdateChannel

Channel::Channel(EventLoop* loop, fd_t fd) : loop_(loop), fd_(fd) {}

Channel::~Channel() {
  // 确保 Channel析构时，它不在 EventLoop 的 Poller 中
  assert(!loop_->HasChannel(this));
}

void Channel::Tie(const std::shared_ptr<void>& obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::Update() {
  // 调用 EventLoop => Poller => epoll_ctl
  loop_->UpdateChannel(this);
}
void Channel::Remove() {
  assert(IsNoneEvent());
  loop_->RemoveChannel(this);
}
// EventLoop 调用此函数
void Channel::HandleEvent(Timestamp receiveTime) {
  std::shared_ptr<void> guard;
  if (tied_) {
    guard = tie_.lock();
    if (guard) {
      HandleEventWithGuard(receiveTime);
    }
    // 如果 guard 为空，说明绑定的对象(如TcpConnection)已销毁，
    // 此时不执行任何回调，安全退出。
  } else {
    HandleEventWithGuard(receiveTime);
  }
}

void Channel::HandleEventWithGuard(Timestamp receiveTime) {
  // 1. 处理连接断开 (EPOLLHUP)
  // 当对端关闭连接时，EPOLLHUP 会触发。
  // 如果没有 EPOLLIN，直接触发关闭；如果有 EPOLLIN，通常会在 ReadCallback 里读到 0 字节后触发关闭。
  if (((revents_ & EPOLLHUP) != 0U) && ((revents_ & EPOLLIN) == 0U)) {
    if (closeCallback_) {
      closeCallback_();
    }
  }

  // 2. 处理错误 (EPOLLERR)
  if ((revents_ & EPOLLERR) != 0U) {
    if (errorCallback_) {
      errorCallback_();
    }
  }

  // 3. 处理读事件 (EPOLLIN | EPOLLPRI | EPOLLRDHUP)
  // EPOLLRDHUP: Stream socket peer closed connection, or shut down writing half of connection.
  if ((revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) != 0U) {
    if (readCallback_) {
      readCallback_(receiveTime);
    }
  }

  // 4. 处理写事件 (EPOLLOUT)
  if ((revents_ & EPOLLOUT) != 0U) {
    if (writeCallback_) {
      writeCallback_();
    }
  }
}