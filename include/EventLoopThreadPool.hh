#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "EventLoopBase.hh"
#include "base/Macros.hh"

class EventLoopBase;
class EventLoopThread;

class EventLoopThreadPool {
public:
  using ThreadInitCallback = std::function<void(EventLoopBase*)>;

  EventLoopThreadPool(EventLoopBase* baseLoop, std::string nameArg);
  ~EventLoopThreadPool() = default;

  void setThreadNum(int numThreads) { numThreads_ = numThreads; }

  void start(const ThreadInitCallback& cb = ThreadInitCallback());

  // 核心方法：获取下一个 Loop (轮询)
  EventLoopBase* getNextLoop();

  std::vector<EventLoopBase*> getAllLoops();

  [[nodiscard]] bool started() const { return started_; }
  [[nodiscard]] const std::string& name() const { return name_; }

private:
  EventLoopBase* baseLoop_;  // 主线程的 Loop
  std::string name_;
  bool started_;
  int numThreads_;
  int next_;  // 轮询的下标

  std::vector<std::unique_ptr<EventLoopThread>> threads_;  // 管理线程对象
  std::vector<EventLoopBase*> loops_;                      // 保存所有子 Loop 的指针
};