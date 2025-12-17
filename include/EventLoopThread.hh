#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "base/Macros.hh"

class EventLoop;

class EventLoopThread {
public:
  using ThreadInitCallback = std::function<void(EventLoop*)>;

  explicit EventLoopThread(ThreadInitCallback cb = ThreadInitCallback(), const std::string& name = std::string());
  ~EventLoopThread();

  EventLoop* startLoop();  // 启动线程，并返回新线程中创建的 Loop 指针

private:
  void threadFunc();  // 线程函数

  EventLoop* loop_;  // 指向新线程中的 loop
  bool exiting_;
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallback callback_;  // 线程初始化回调
};