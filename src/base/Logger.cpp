#include "Logger.hh"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

const char* LogLevelToString(LogLevel level) {
  switch (level) {
    case LogLevel::DEBUG:
      return "DEBUG";
    case LogLevel::INFO:
      return "INFO";
    case LogLevel::WARNING:
      return "WARN";
    case LogLevel::ERROR:
      return "ERROR";
    case LogLevel::FATAL:
      return "FATAL";
    default:
      return "UNKNOWN";
  }
}

Logger::Logger() : logLevel_(LogLevel::INFO) {
  outputFunc_ = [](std::string_view buffer) { fwrite(buffer.data(), buffer.size(), 1, stdout); };
}

void Logger::initAsyncLog(std::string_view path) {
  asyncLogger_ = std::make_unique<AsyncLogger>(path);
  outputFunc_ = [this](std::string_view buffer) { asyncLogger_->log(buffer); };
}

void Logger::setLogLevel(LogLevel level) {
  logLevel_ = level;
}
void Logger::setOutput(OutputFunc func) {
  outputFunc_ = std::move(func);
}

void Logger::log(LogLevel level, const char* file, int line, const char* format, ...) {
  if (level < logLevel_) {
    return;
  }
  char buf[1024];  // NOLINT
  size_t offset = 0;
  time_t now = time(nullptr);
  struct tm tm_time;
  localtime_r(&now, &tm_time);
  offset += strftime(buf + offset, sizeof(buf) - offset, "%Y-%m-%d %H:%M:%S ", &tm_time);
  offset += snprintf(buf + offset, sizeof(buf) - offset, "[%s] [%s:%d] ", LogLevelToString(level), file, line);
  va_list args;
  va_start(args, format);
  offset += vsnprintf(buf + offset, sizeof(buf) - offset, format, args);
  va_end(args);
  if (offset < static_cast<int>(sizeof(buf)) - 2) {
    buf[offset++] = '\n';
    buf[offset] = '\0';
  }

  if (outputFunc_) {
    outputFunc_(std::string_view(buf, offset));
  }

  if (level == LogLevel::FATAL) {
    abort();
  }
}