#pragma once

#include <cstdint>
#include <string>
class HttpResponse {
public:
private:
  int status_code_;
  std::string status_message_;
  std::string body_;
};