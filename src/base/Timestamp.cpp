#include "Timestamp.hh"

#include <sys/time.h>

#include <array>
#include <cstdio>  // for snprintf

static constexpr int64_t kMicroSecondsPerSecond = 1000000;
static constexpr int kYearsBase = 1900;
static constexpr int kMonthsBase = 1;
Timestamp::Timestamp() : microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch) : microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

Timestamp Timestamp::Now() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  int64_t seconds = tv.tv_sec;
  return Timestamp((seconds * kMicroSecondsPerSecond) + tv.tv_usec);
}

Timestamp Timestamp::Invalid() {
  return {};
}

std::string Timestamp::ToString() const {
  static constexpr size_t kTimestampBufferSize = 64;
  std::array<char, kTimestampBufferSize> buf{};
  auto seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
  struct tm tm_time;
  gmtime_r(&seconds, &tm_time);

  int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
  snprintf(buf.data(), buf.size(), "%4d%02d%02d %02d:%02d:%02d.%06d", tm_time.tm_year + kYearsBase,
           tm_time.tm_mon + kMonthsBase, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
           microseconds);
  return {buf.data()};
}