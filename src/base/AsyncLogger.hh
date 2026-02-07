#pragma once
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "base/Macros.hh"

class AsyncLogger {
private:
  static constexpr size_t BUFFER_SIZE = static_cast<const size_t>(1024 * 1024);

  struct Buffer {
    char data[BUFFER_SIZE];  // NOLINT
    size_t current{0};

    [[nodiscard]] size_t avail() const { return BUFFER_SIZE - current; }
    void append(const char* msg, size_t len) {
      std::memcpy(data + current, msg, len);
      current += len;
    }
    void clear() { current = 0; }
    [[nodiscard]] bool empty() const { return current == 0; }
  };

  using BufferPtr = std::unique_ptr<Buffer>;

public:
  explicit AsyncLogger(std::string_view filename);
  ~AsyncLogger();

  DISALLOW_COPY(AsyncLogger);
  void log(std::string_view sv) { log(sv.data(), sv.size()); }
  void log(const char* msg, size_t len);

private:
  void backendThread();

  std::atomic<bool> running_{true};
  std::string filename_;
  std::ofstream file_;
  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;

  BufferPtr currentBuffer_;         // 当前缓冲区
  BufferPtr nextBuffer_;            // 预备缓冲区
  std::vector<BufferPtr> buffers_;  // 待写入文件的缓冲区队列
};

#define ASYNC_LOG(logger, format, ...)                         \
  do {                                                         \
    char buf[1024];                                            \
    int n = snprintf(buf, sizeof(buf), format, ##__VA_ARGS__); \
    if (n > 0) logger.log(buf, static_cast<size_t>(n));        \
  } while (0)