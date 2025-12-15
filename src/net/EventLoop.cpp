#include "EventLoop.hh"

#include <sys/eventfd.h>
#include <unistd.h>

#include <cassert>

#include "Channel.hh"
#include "Epoll.hh"
#include "base/Timestamp.hh"

static fd_t CreateEventfd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    perror("eventfd error");
    abort();
  }
  return evtfd;
}

EventLoop::EventLoop()
    : tid_(CurrentThread::tid()),
      epoll_(std::make_unique<Epoll>()),
      wakeupFd_(CreateEventfd()),
      wakeupChannel_(std::make_unique<Channel>(this, wakeupFd_)) {
  // 绑定读回调
  wakeupChannel_->SetReadCallback([this](Timestamp) { HandleRead(); });
  // 注册到 Epoll，开始监听读事件
  wakeupChannel_->EnableRead();
}

EventLoop::~EventLoop() {
  // 先 disable，再 remove，最后 close fd
  wakeupChannel_->DisableAll();
  wakeupChannel_->Remove();
  ::close(wakeupFd_);
}

void EventLoop::Loop() {
  looping_.store(true);
  quit_.store(false);

  while (!quit_) {
    activeChannels_.clear();
    // 阻塞等待事件
    auto ts = epoll_->poll(kPollTimeMs, &activeChannels_);

    for (auto *channel : activeChannels_) {
      channel->HandleEvent(ts);
    }
    // 执行任务队列
    DoPendingFunctors();
  }
  looping_.store(false);
}

void EventLoop::Quit() {
  quit_.store(true);
  // 如果是在其他线程调用 Quit，必须唤醒 Loop 才能让它跳出 while 检查 quit_
  if (!IsInEventLoopThread()) {
    Wakeup();
  }
}

void EventLoop::RunInLoop(Functor cb) {
  if (IsInEventLoopThread()) {
    cb();
  } else {
    QueueInLoop(std::move(cb));
  }
}

void EventLoop::Wakeup() const {
  uint64_t one = 1;
  ssize_t n = ::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof(one)) {
    perror("EventLoop::Wakeup() writes mismatch");
  }
}

void EventLoop::HandleRead() const {
  uint64_t one = 1;
  ssize_t n = ::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof(one)) {
    perror("EventLoop::HandleRead() reads mismatch");
  }
}

void EventLoop::UpdateChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  epoll_->UpdateChannel(channel);
}

void EventLoop::RemoveChannel(Channel *channel) {
  assert(channel->ownerLoop() == this);
  epoll_->RemoveChannel(channel);
}
bool EventLoop::HasChannel(Channel *channel) const {
  return epoll_->HasChannel(channel);
}
void EventLoop::QueueInLoop(Functor cb) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pendingFunctors_.emplace_back(std::move(cb));
  }

  // 1. 跨线程调用：必须唤醒
  // 2. 当前线程调用但正在执行回调：必须唤醒，否则新任务要等到下一轮 epoll_wait 超时才执行
  if (!IsInEventLoopThread() || callingPendingFunctors_) {
    Wakeup();
  }
}

void EventLoop::DoPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    std::scoped_lock<std::mutex> lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (const auto &functor : functors) {
    functor();
  }
  callingPendingFunctors_ = false;
}