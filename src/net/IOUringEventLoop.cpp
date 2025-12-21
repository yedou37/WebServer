#include "IOUringEventLoop.hh"

#include <sys/eventfd.h>
#include <unistd.h>

#include <cassert>

#include "Channel.hh"
#include "base/Timestamp.hh"

#ifdef USE_IO_URING
#include <liburing.h>
#endif

static int CreateEventfd() {
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0) {
    perror("Failed in eventfd");
    abort();
  }
  return evtfd;
}

IOUringEventLoop::IOUringEventLoop()
    : tid_(CurrentThread::tid()),
      wakeupFd_(CreateEventfd()),
      wakeupChannel_(std::make_unique<Channel>(this, wakeupFd_)) {
#ifdef USE_IO_URING
  // Initialize io_uring
  int ret = io_uring_queue_init(1024, &ring_, 0);
  if (ret < 0) {
    perror("io_uring_queue_init failed");
    abort();
  }
#endif

  // 绑定读回调
  wakeupChannel_->SetReadCallback([this](Timestamp) { HandleRead(); });
  // 注册到 io_uring，开始监听读事件
  wakeupChannel_->EnableRead();
}

IOUringEventLoop::~IOUringEventLoop() {
  // 先 disable，再 remove，最后 close fd
  wakeupChannel_->DisableAll();
  wakeupChannel_->Remove();
  ::close(wakeupFd_);
  
#ifdef USE_IO_URING
  io_uring_queue_exit(&ring_);
#endif
}

void IOUringEventLoop::Loop() {
  assert(!looping_);
  assert(IsInEventLoopThread());
  looping_.store(true);
  quit_.store(false);

#ifdef USE_IO_URING
  while (!quit_) {
    // Submit all queued operations
    io_uring_submit(&ring_);
    
    // Process completion events
    struct io_uring_cqe* cqe;
    unsigned head;
    int count = 0;
    
    // Process all completed events
    io_uring_for_each_cqe(&ring_, head, cqe) {
      // Handle completion event
      // This is where we would process accept, read, write completions
      count++;
    }
    
    // Advance the completion queue
    if (count > 0) {
      io_uring_cq_advance(&ring_, count);
    }
    
    // Execute pending functors
    DoPendingFunctors();
  }
#endif

  looping_.store(false);
}

void IOUringEventLoop::Quit() {
  quit_.store(true);
  // 如果是在其他线程调用 Quit，必须唤醒
  // 否则需要等到下一轮 poll 超时才退出
  if (!IsInEventLoopThread()) {
    Wakeup();
  }
}

void IOUringEventLoop::RunInLoop(Functor cb) {
  if (IsInEventLoopThread()) {
    cb();
  } else {
    QueueInLoop(std::move(cb));
  }
}

void IOUringEventLoop::Wakeup() const {
  uint64_t one = 1;
  ssize_t n = ::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof(one)) {
    perror("IOUringEventLoop::Wakeup() writes");
  }
}

void IOUringEventLoop::HandleRead() const {
  uint64_t one = 1;
  ssize_t n = ::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof(one)) {
    perror("IOUringEventLoop::HandleRead() reads");
  }
}

void IOUringEventLoop::UpdateChannel(Channel* channel) {
  assert(channel->ownerLoop() == this);
  // In io_uring mode, we would submit the operation to the ring
}

void IOUringEventLoop::RemoveChannel(Channel* channel) {
  assert(channel->ownerLoop() == this);
  // In io_uring mode, we would cancel pending operations for this channel
}

bool IOUringEventLoop::HasChannel(Channel* channel) const {
  // Check if channel is registered with this event loop
  return true; // Simplified implementation
}

void IOUringEventLoop::QueueInLoop(Functor cb) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pendingFunctors_.emplace_back(std::move(cb));
  }

  // 1. 跨线程调用：必须唤醒
  // 2. 当前线程调用但正在执行回调：必须唤醒，否则新任务要等到下一轮处理才执行
  if (!IsInEventLoopThread() || callingPendingFunctors_) {
    Wakeup();
  }
}

void IOUringEventLoop::DoPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    std::scoped_lock<std::mutex> lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (const auto& functor : functors) {
    functor();
  }
  callingPendingFunctors_ = false;
}