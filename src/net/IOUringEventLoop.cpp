#include "IOUringEventLoop.hh"

#include <liburing.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <iostream>
#include <vector>

#include "Channel.hh"
#include "base/Timestamp.hh"

namespace {
// 1. 队列深度
constexpr int kIOURingQueueSize = 32768;

// 2. 无效 UserData 标识 (用于忽略 Remove 操作的完成通知)
constexpr uint64_t kIgnoreUserData = static_cast<uint64_t>(-1);

// 3. 确保宏定义存在 (兼容旧系统头文件)
#ifndef IORING_POLL_ADD_MULTI
#define IORING_POLL_ADD_MULTI (1U << 0)
#endif
}  // namespace

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
  // 初始化 io_uring
  int ret = io_uring_queue_init(kIOURingQueueSize, &ring_, IORING_SETUP_CLAMP);
  if (ret < 0) {
    perror("io_uring_queue_init failed");
    abort();
  }

  // 绑定 wakeupChannel
  wakeupChannel_->SetReadCallback([this](Timestamp) { HandleRead(); });
  wakeupChannel_->EnableRead();
}

IOUringEventLoop::~IOUringEventLoop() {
  // 1. 移除所有 Channel 的监听
  for (auto& pair : channels_) {
    Channel* channel = pair.second;
    struct io_uring_sqe* sqe = GetSqe();
    if (sqe != nullptr) {
      io_uring_prep_poll_remove(sqe, static_cast<uint64_t>(channel->Getfd()));
      io_uring_sqe_set_data64(sqe, kIgnoreUserData);
    }
  }

  // 2. 提交并等待清理完成
  io_uring_submit(&ring_);

  // 简单的清理等待逻辑
  // 注意：在析构中严格处理 CQE 可能比较复杂，这里主要确保内核不再持有引用
  struct io_uring_cqe* cqe;
  while (io_uring_peek_cqe(&ring_, &cqe) == 0) {
    io_uring_cqe_seen(&ring_, cqe);
  }

  wakeupChannel_->DisableAll();
  wakeupChannel_->Remove();
  ::close(wakeupFd_);
  io_uring_queue_exit(&ring_);
}

// 阻塞式获取 SQE，确保绝不丢弃操作
struct io_uring_sqe* IOUringEventLoop::GetSqe() {
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  while (sqe == nullptr) {
    // 队列满了，提交任务腾出空间
    io_uring_submit(&ring_);
    sqe = io_uring_get_sqe(&ring_);
  }
  return sqe;
}

void IOUringEventLoop::Loop() {  // NOLINT
  assert(!looping_);
  assert(IsInEventLoopThread());
  looping_ = true;
  quit_ = false;

  while (!quit_) {
    // 提交并等待至少 1 个事件
    int ret = io_uring_submit_and_wait(&ring_, 1);
    if (ret < 0) {
      if (ret == -EINTR) {
        continue;
      }
      perror("io_uring_submit_and_wait");
      break;  // 严重错误退出
    }

    struct io_uring_cqe* cqes[128];                               // NOLINT
    unsigned count = io_uring_peek_batch_cqe(&ring_, cqes, 128);  // NOLINT

    for (unsigned i = 0; i < count; ++i) {
      struct io_uring_cqe* cqe = cqes[i];
      uint64_t user_data = cqe->user_data;

      // 1. 忽略我们主动发起的 Remove 操作的完成通知
      if (user_data == kIgnoreUserData) {
        continue;
      }

      // 2. 通过 fd 查找 Channel
      int fd = static_cast<int>(user_data);
      auto it = channels_.find(fd);

      // 如果 Channel 已经被销毁/移除，忽略该事件
      if (it == channels_.end()) {
        continue;
      }
      Channel* channel = it->second;

      // 3. 处理事件结果
      if (cqe->res < 0) {
        // -ECANCELED 是正常的 (例如 Close 或者 UpdateChannel 时的 Remove)
        if (cqe->res != -ECANCELED) {
          // 可以在此记录日志
        }
      } else {
        int revents = cqe->res;
        channel->SetRevents(revents);
        channel->HandleEvent(Timestamp::Now());

        // 【关键】Multi-Shot 模式下，内核会自动保持监听，
        // 我们不需要像 Epoll 那样重新 epoll_ctl，也不需要重新 io_uring_prep_poll_add。
        // 代码执行到这里，Channel 依然处于监听状态。
      }
    }

    // 推进 CQ 队列
    if (count > 0) {
      io_uring_cq_advance(&ring_, count);
    }

    // 执行 pending 任务 (如 runInLoop)
    DoPendingFunctors();
  }

  looping_ = false;
}

void IOUringEventLoop::Quit() {
  quit_ = true;
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
  (void)n;
}

void IOUringEventLoop::UpdateChannel(Channel* channel) {
  assert(channel->ownerLoop() == this);

  // 步骤 1: 总是尝试移除旧的 Poll 请求
  // 这是为了应对从 Read 变成 Write 等掩码变化的情况，或者避免重复添加。
  // 即使该 fd 之前不在 ring 中，Remove 失败 (ENOENT) 也不会有副作用，
  // 因为我们在 Loop 中忽略了 kIgnoreUserData 的结果。
  struct io_uring_sqe* sqe_remove = GetSqe();
  io_uring_prep_poll_remove(sqe_remove, static_cast<uint64_t>(channel->Getfd()));
  io_uring_sqe_set_data64(sqe_remove, kIgnoreUserData);

  // 步骤 2: 更新映射表
  channels_[channel->Getfd()] = channel;

  // 步骤 3: 如果没有任何感兴趣的事件，到此为止 (已完成移除)
  if (channel->IsNoneEvent()) {
    return;
  }

  // 步骤 4: 准备新的事件掩码
  int16_t events = 0;
  if (channel->IsReading()) {
    events |= POLLIN;
  }
  if (channel->IsWriting()) {
    events |= POLLOUT;
  }
  events |= (POLLRDHUP | POLLPRI | POLLERR | POLLHUP);

  // 步骤 5: 添加新的 Poll 请求 (启用 Multi-Shot)
  struct io_uring_sqe* sqe_add = GetSqe();
  io_uring_prep_poll_add(sqe_add, channel->Getfd(), events);

  // 【核心修复】开启 Multi-Shot 模式
  // 类似于 epoll 的持久监听。如果不设置这个，第一次触发后监听就会消失。
  sqe_add->len |= IORING_POLL_ADD_MULTI;

  io_uring_sqe_set_data64(sqe_add, static_cast<uint64_t>(channel->Getfd()));
}

void IOUringEventLoop::RemoveChannel(Channel* channel) {
  assert(channel->ownerLoop() == this);

  // 1. 从映射中删除，防止后续迟到的 CQE 触发回调
  channels_.erase(channel->Getfd());

  // 2. 向内核提交移除请求
  struct io_uring_sqe* sqe = GetSqe();
  io_uring_prep_poll_remove(sqe, static_cast<uint64_t>(channel->Getfd()));
  io_uring_sqe_set_data64(sqe, kIgnoreUserData);
}

bool IOUringEventLoop::HasChannel(Channel* channel) const {
  auto it = channels_.find(channel->Getfd());
  return it != channels_.end() && it->second == channel;
}

void IOUringEventLoop::QueueInLoop(Functor cb) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pendingFunctors_.emplace_back(std::move(cb));
  }

  if (!IsInEventLoopThread() || callingPendingFunctors_) {
    Wakeup();
  }
}

void IOUringEventLoop::DoPendingFunctors() {
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (const auto& functor : functors) {
    functor();
  }
  callingPendingFunctors_ = false;
}