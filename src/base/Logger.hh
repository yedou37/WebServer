#pragma once

#include <fstream>
#include <functional>
#include <memory>
#include <string>

#include "../base/Macros.hh"
#include "LogLevel.hh"

class Logger {
public:
  using OutputFunc = std::function<void(LogLevel, const std::string&)>;
  static Logger& instance() {
    static Logger logger;
    return logger;
  };
  void setLogLevel(LogLevel level);
  void setOutput(OutputFunc func);
  void log(LogLevel level, const char* file, int line, const char* format, ...);

private:
  Logger();
  ~Logger();
  DISALLOW_COPY_AND_MOVE(Logger);
  LogLevel logLevel_;
  OutputFunc outputFunc_;
  std::ofstream fileStream_;
};

#define LOG_DEBUG(format, ...) Logger::instance().log(LogLevel::DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_INFO(format, ...) Logger::instance().log(LogLevel::INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_WARN(format, ...) Logger::instance().log(LogLevel::WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_ERROR(format, ...) Logger::instance().log(LogLevel::ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_FATAL(format, ...) Logger::instance().log(LogLevel::FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__)