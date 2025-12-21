#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "EventLoopFactory.hh"
#include "base/Macros.hh"

class EventLoopBase;

class EventLoopThread {
public:
  using ThreadInitCallback = std::function<void(EventLoopBase*)>;

  explicit EventLoopThread(ThreadInitCallback cb = ThreadInitCallback(), const std::string& name = std::string(),
                           EventLoopType type = EventLoopType::EPOLL);
  ~EventLoopThread();

  EventLoopBase* startLoop();  // 启动线程，并返回新线程中创建的 Loop 指针

private:
  void threadFunc();  // 线程函数

  EventLoopBase* loop_;  // 指向新线程中的 loop
  bool exiting_;
  EventLoopType loop_type_;
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallback callback_;  // 线程初始化回调
};