#pragma once
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class AsyncLogger {
private:
  static constexpr size_t BUFFER_SIZE = 4096;

  struct Buffer {
    char data[BUFFER_SIZE];  // NOLINT
    size_t current{0};

    bool append(const char* msg, size_t len) {
      if (current + len > BUFFER_SIZE) {
        return false;  // 缓冲区满了
      }
      memcpy(data + current, msg, len);
      current += len;
      return true;
    }

    void clear() { current = 0; }
  };

public:
  explicit AsyncLogger(const std::string& filename);
  ~AsyncLogger();

  void log(const std::string& message);

private:
  void backendThread();

  // 双缓冲区
  Buffer bufferA_;
  Buffer bufferB_;
  Buffer* currentBuffer_;  // 前台正在写的缓冲区
  Buffer* backupBuffer_;   // 后台准备写的缓冲区

  // 同步机制
  std::mutex mutex_;
  std::condition_variable condition_;
  bool quit_ = false;

  // 后台线程
  std::thread backendThread_;
  std::ofstream file_;
};

// 便捷宏
#define ASYNC_LOG(logger, format, ...)                       \
  do {                                                       \
    char buffer[1024];                                       \
    snprintf(buffer, sizeof(buffer), format, ##__VA_ARGS__); \
    logger.log(std::string(buffer));                         \
  } while (0)