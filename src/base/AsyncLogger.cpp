#include "AsyncLogger.hh"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string_view>

AsyncLogger::AsyncLogger(std::string_view filename)
    : filename_(filename),
      file_(std::filesystem::path(filename), std::ios::app),
      currentBuffer_(std::make_unique<Buffer>()),
      nextBuffer_(std::make_unique<Buffer>()) {
  if (!file_.is_open()) {
    throw std::runtime_error("Failed to open log file");
  }
  thread_ = std::thread(&AsyncLogger::backendThread, this);
}

AsyncLogger::~AsyncLogger() {
  running_ = false;
  cond_.notify_all();
  if (thread_.joinable()) {
    thread_.join();
  }
}

void AsyncLogger::log(const char* msg, size_t len) {
  std::lock_guard<std::mutex> lock(mutex_);
  len = std::min(len, BUFFER_SIZE);  // 截断
  if (currentBuffer_->avail() >= len) {
    currentBuffer_->append(msg, len);
  } else {
    buffers_.push_back(std::move(currentBuffer_));
    if (nextBuffer_) {
      currentBuffer_ = std::move(nextBuffer_);
    } else {
      // 极罕见情况：后台线程太慢，预备缓冲区还没回来，被迫新开内存
      currentBuffer_ = std::make_unique<Buffer>();
    }

    currentBuffer_->append(msg, len);
    cond_.notify_one();  // 通知后台线程写盘
  }
}

void AsyncLogger::backendThread() {
  // 后台线程私有的缓冲区，用于交换
  BufferPtr b1 = std::make_unique<Buffer>();
  BufferPtr b2 = std::make_unique<Buffer>();
  std::vector<BufferPtr> buffersToWrite;
  buffersToWrite.reserve(16);  // NOLINT

  while (running_) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      if (buffers_.empty()) {
        // 如果没有待写数据，最多等 3 秒强制刷新一次
        cond_.wait_for(lock, std::chrono::seconds(3));
      }

      // 将当前正在写的缓冲区也推入队列（即使没满，为了保证实时性）
      buffers_.push_back(std::move(currentBuffer_));

      // 归还新的缓冲区给前台
      currentBuffer_ = std::move(b1);
      if (!nextBuffer_) {
        nextBuffer_ = std::move(b2);
      }

      // 核心操作：交换整个待写队列到局部变量，缩短持锁时间
      buffersToWrite.swap(buffers_);
    }

    // --- 锁外 IO 开始 ---
    for (const auto& buffer : buffersToWrite) {
      if (!buffer->empty()) {
        file_.write(buffer->data, static_cast<int64_t>(buffer->current));
      }
    }
    file_.flush();

    // 重新填充私有缓冲区 (重用内存)
    if (buffersToWrite.size() > 2) {
      // 如果写了很多，只保留两个循环使用
      buffersToWrite.resize(2);
    }

    if (!b1) {
      b1 = std::move(buffersToWrite.back());
      b1->clear();
      buffersToWrite.pop_back();
    }
    if (!b2 && !buffersToWrite.empty()) {
      b2 = std::move(buffersToWrite.back());
      b2->clear();
      buffersToWrite.pop_back();
    }

    buffersToWrite.clear();
    // --- 锁外 IO 结束 ---

    if (!running_) {
      // 退出前最后一次检查由析构函数触发，
      // 此时 currentBuffer_ 已在上面的逻辑中被推入 buffersToWrite 并处理
      break;
    }
  }
  file_.close();
}