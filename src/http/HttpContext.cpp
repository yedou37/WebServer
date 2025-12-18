#include "http/HttpContext.hh"

#include <string_view>
bool HttpContext::parseRequestLine(const char* begin, const char* end) {
  bool succeed{false};
  const char* start = begin;
  const char* space = std::find(start, end, ' ');
  if (space != end && request_.SetMethod(start, space)) {
    // successfully set method
    start = space + 1;
    space = std::find(start, end, ' ');
    if (space != end) {
      const char* question = std::find(start, space, '?');
      if (question != space) {
        request_.SetPath(start, question);
        request_.SetQuery(question + 1, space);
      } else {
        request_.SetPath(start, space);
      }
      start = space + 1;
      succeed = (end - start == 8) && std::equal(start, end - 1, "HTTP/1.");  // NOLINT
      if (succeed) {
        if (*(end - 1) == '1') {
          request_.SetVersion(HttpRequest::Version::HTTP_1_1);
        } else if (*(end - 1) == '0') {
          request_.SetVersion(HttpRequest::Version::HTTP_1_0);
        } else {
          request_.SetVersion(HttpRequest::Version::UNKNOWN);
          succeed = false;
        }
      }
    }
  }
  return succeed;
}
HttpContext::LineStatus HttpContext::processRequestLine(Buffer* buf) {
  const char* end = buf->peek() + buf->readableBytes();
  const char* crlf = std::search(buf->peek(), end, kCRLF, kCRLF + 2);

  if (crlf == end) {
    return LineStatus::MORE_DATA;
  }

  if (!parseRequestLine(buf->peek(), crlf)) {
    return LineStatus::BAD_REQUEST;
  }

  buf->retrieve(crlf + 2 - buf->peek());
  state_ = HttpRequestParseState::EXPECT_HEADERS;
  return LineStatus::OK;
}

HttpContext::LineStatus HttpContext::processHeaders(Buffer* buf) {
  const char* end = buf->peek() + buf->readableBytes();
  const char* crlf = std::search(buf->peek(), end, kCRLF, kCRLF + 2);

  if (crlf == end) {
    return LineStatus::MORE_DATA;
  }

  const char* colon = std::find(buf->peek(), crlf, ':');
  if (colon != crlf) {
    std::string_view headerLine(buf->peek(), crlf - buf->peek());

    std::string_view key = headerLine.substr(0, colon - buf->peek());

    size_t colonIdx = colon - buf->peek();
    size_t valStart = colonIdx + 1;
    while (valStart < headerLine.size() && (isspace(headerLine[valStart]) != 0)) {
      valStart++;
    }
    std::string_view value = headerLine.substr(valStart);

    request_.AddHeader(key, value);

  } else {
    // 空行，Header 结束
    state_ = HttpRequestParseState::GOT_ALL;
  }

  buf->retrieve(crlf + 2 - buf->peek());
  return LineStatus::OK;
}

bool HttpContext::ParseRequest(Buffer* buf, Timestamp receiveTime) {
  bool ok = true;
  bool hasMore = true;

  while (hasMore) {
    if (state_ == HttpRequestParseState::EXPECT_REQUEST_LINE) {
      LineStatus status = processRequestLine(buf);
      if (status == LineStatus::OK) {
      } else if (status == LineStatus::MORE_DATA) {
        hasMore = false;  // 等待下次 TCP 数据
      } else {            // BAD_REQUEST
        ok = false;
        hasMore = false;
      }
    } else if (state_ == HttpRequestParseState::EXPECT_HEADERS) {
      LineStatus status = processHeaders(buf);
      if (status == LineStatus::OK) {
        if (state_ == HttpRequestParseState::GOT_ALL) {
          hasMore = false;  // 解析完成
        }
        // 否则继续循环解析下一行 Header
      } else if (status == LineStatus::MORE_DATA) {
        hasMore = false;
      } else {
        ok = false;
        hasMore = false;
      }
    } else if (state_ == HttpRequestParseState::GOT_ALL) {
      hasMore = false;
    }
  }
  return ok;
}