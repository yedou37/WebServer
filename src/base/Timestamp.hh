#pragma once

#include <iostream>
#include <string>

class Timestamp {
public:
  Timestamp();
  explicit Timestamp(int64_t microSecondsSinceEpoch);

  static Timestamp Now();
  static Timestamp Invalid();

  [[nodiscard]] std::string ToString() const;
  [[nodiscard]] bool Valid() const { return microSecondsSinceEpoch_ > 0; }
  [[nodiscard]] int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

  bool operator<(const Timestamp& rhs) const { return microSecondsSinceEpoch_ < rhs.microSecondsSinceEpoch_; }
  bool operator==(const Timestamp& rhs) const { return microSecondsSinceEpoch_ == rhs.microSecondsSinceEpoch_; }

private:
  int64_t microSecondsSinceEpoch_;
};