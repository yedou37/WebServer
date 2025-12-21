#include "EventLoopThreadPool.hh"

#include <cassert>
#include <utility>

#include "EventLoop.hh"
#include "EventLoopThread.hh"

EventLoopThreadPool::EventLoopThreadPool(EventLoopBase* baseLoop, std::string nameArg)
    : baseLoop_(baseLoop), name_(std::move(nameArg)), started_(false), numThreads_(0), next_(0) {}

void EventLoopThreadPool::start(const ThreadInitCallback& cb) {
  assert(!started_);
  assert(baseLoop_->IsInEventLoopThread() == true);

  started_ = true;

  // 创建指定数量的线程
  for (int i = 0; i < numThreads_; ++i) {
    std::vector<char> buf(name_.size() + 32);  // NOLINT
    snprintf(buf.data(), buf.size(), "%s%d", name_.c_str(), i);
    threads_.emplace_back(std::make_unique<EventLoopThread>(cb, std::string(buf.data())));

    // 启动线程并获取 Loop 指针
    loops_.push_back(threads_.back()->startLoop());
  }

  // 如果没有设置子线程，那么 loops_ 为空，getNextLoop 应该返回 baseLoop_
  if (numThreads_ == 0 && cb) {
    cb(baseLoop_);
  }
}

EventLoopBase* EventLoopThreadPool::getNextLoop() {
  assert(baseLoop_->IsInEventLoopThread() == true);
  assert(started_);
  EventLoopBase* loop = baseLoop_;

  // 轮询算法 (Round-Robin)
  if (!loops_.empty()) {
    loop = loops_[next_];
    ++next_;
    if (static_cast<size_t>(next_) >= loops_.size()) {
      next_ = 0;
    }
  }
  return loop;
}

std::vector<EventLoopBase*> EventLoopThreadPool::getAllLoops() {
  assert(baseLoop_->IsInEventLoopThread() == true);
  assert(started_);
  if (loops_.empty()) {
    return std::vector<EventLoopBase*>(1, baseLoop_);
  }
  return loops_;
}