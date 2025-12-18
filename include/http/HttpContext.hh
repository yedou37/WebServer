#pragma once

#include <cstdint>

#include "Buffer.hh"
#include "HttpRequest.hh"
#include "base/Timestamp.hh"

class HttpContext {
public:
  constexpr static char kCRLF[] = "\r\n";  // NOLINT

  enum class HttpRequestParseState : std::uint8_t {
    EXPECT_REQUEST_LINE,  // 正在等待请求行
    EXPECT_HEADERS,       // 正在等待头部
    EXPECT_BODY,          // 正在等待包体
    GOT_ALL,              // 解析完毕
  };

  enum class LineStatus : std::uint8_t { OK, MORE_DATA, BAD_REQUEST };

  HttpContext() = default;
  ~HttpContext() = default;

  bool ParseRequest(Buffer* buf, Timestamp receiveTime);

  [[nodiscard]] bool IsGotAll() const { return state_ == HttpRequestParseState::GOT_ALL; };

  void reset();

  [[nodiscard]] const HttpRequest& request() const { return request_; };
  [[nodiscard]] HttpRequest& request() { return request_; };

private:
  bool parseRequestLine(const char* begin, const char* end);
  LineStatus processRequestLine(Buffer* buf);
  LineStatus processHeaders(Buffer* buf);

  HttpRequestParseState state_{HttpRequestParseState::EXPECT_REQUEST_LINE};
  HttpRequest request_;
};