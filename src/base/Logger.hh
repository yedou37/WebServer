#pragma once

#include <fstream>
#include <functional>
#include <memory>
#include <string_view>

#include "../base/Macros.hh"
#include "LogLevel.hh"
#include "base/AsyncLogger.hh"

class Logger {
public:
  using OutputFunc = std::function<void(std::string_view)>;
  static Logger& instance() {
    static Logger logger;
    return logger;
  }
  void initAsyncLog(std::string_view path);
  void setLogLevel(LogLevel level);
  void setOutput(OutputFunc func);
  void log(LogLevel level, const char* file, int line, const char* format, ...);
  [[nodiscard]] LogLevel getLogLevel() const { return logLevel_; }

private:
  Logger();
  ~Logger() = default;
  DISALLOW_COPY_AND_MOVE(Logger);
  LogLevel logLevel_;
  OutputFunc outputFunc_;

  std::unique_ptr<AsyncLogger> asyncLogger_;
};

#define LOG_BASE(level, format, ...)                                                                    \
  do {                                                                                                  \
    if (Logger::instance().getLogLevel() <= level) {                                                    \
      const char* file = __FILE__;                                                                      \
      const char* lastSlash = strrchr(file, '/');                                                       \
      Logger::instance().log(level, lastSlash ? lastSlash + 1 : file, __LINE__, format, ##__VA_ARGS__); \
    }                                                                                                   \
  } while (0)

// 用户调用的宏
#define LOG_DEBUG(format, ...) LOG_BASE(LogLevel::DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...) LOG_BASE(LogLevel::INFO, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...) LOG_BASE(LogLevel::WARN, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG_BASE(LogLevel::ERROR, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...) LOG_BASE(LogLevel::FATAL, format, ##__VA_ARGS__)