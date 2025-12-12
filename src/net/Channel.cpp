#include "Channel.hh"

Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd) {}

Channel::~Channel() {
  assert(!loop_->HasChannel(this));
}
void Channel::Tie(const std::shared_ptr<void>& obj) {
  tie_ = obj;
  tied_ = true;
}
void Channel::HandleEvent() {
  std::shared_ptr<void> guard;
  if (tied_) {
    guard = tie_.lock();
    if (guard) {
      HandleEventWithGuard();
    }
    // 如果 guard 为空，说明拥有者已经释放，不再处理事件，防止 crash
  } else {
    HandleEventWithGuard();
  }
}

void Channel::HandleEventWithGuard() {
  // 1. 处理连接断开 (EPOLLHUP)
  // 通常 EPOLLHUP 时也可能伴随 EPOLLIN，所以要把数据读完。
  if (((revents_ & EPOLLHUP) != 0U) && ((revents_ & EPOLLIN) == 0U)) {
    if (closeCallback_) {
      closeCallback_();
    }
  }

  // 2. 处理错误 (EPOLLERR)
  if ((revents_ & kErrorEvent) != 0U) {
    if (errorCallback_) {
      errorCallback_();
    }
  }

  // 3. 处理读事件 (EPOLLIN | EPOLLPRI | EPOLLRDHUP)
  if ((revents_ & kReadEvent) != 0U) {
    if (readCallback_) {
      readCallback_();
    }
  }

  // 4. 处理写事件 (EPOLLOUT)
  if ((revents_ & kWriteEvent) != 0U) {
    if (writeCallback_) {
      writeCallback_();
    }
  }
}
void Channel::Update() {
  loop_->UpdateChannel(this);
}