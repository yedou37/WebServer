#include "EventLoopThread.hh"

#include "EventLoop.hh"

EventLoopThread::EventLoopThread(ThreadInitCallback cb, const std::string& name, EventLoopType type)
    : loop_(nullptr), exiting_(false), loop_type_(type), callback_(std::move(cb)) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;
  if (loop_ != nullptr) {
    loop_->Quit();  // 退出循环
    thread_.join();
  }
}

// 这个方法由【主线程】调用
EventLoopBase* EventLoopThread::startLoop() {
  // 启动新线程，执行 threadFunc
  thread_ = std::thread([this]() { threadFunc(); });

  EventLoopBase* loop = nullptr;
  {
    // 等待新线程创建 EventLoop 完毕
    std::unique_lock<std::mutex> lock(mutex_);
    while (loop_ == nullptr) {
      cond_.wait(lock);
    }
    loop = loop_;
  }
  return loop;
}

// 这个方法在【新线程】中运行
void EventLoopThread::threadFunc() {
  auto loop_ptr = EventLoopFactory::Create(loop_type_);
  EventLoopBase* loop = loop_ptr.get();

  if (callback_) {
    callback_(loop);
  }

  {
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = loop;        // 获取指针
    cond_.notify_one();  // 通知主线程：Loop 创建好了，你可以拿走了
  }

  loop->Loop();  // 开始事件循环，这里会一直阻塞，直到 Quit()

  // 循环结束，清理指针
  std::lock_guard<std::mutex> lock(mutex_);
  loop_ = nullptr;
}