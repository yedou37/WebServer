#include "http/HttpResponse.hh"

#include <cstdio>

void HttpResponse::AppendToBuffer(Buffer* output) const {
  char buf[32];  // NOLINT

  // 1. 写入请求行: HTTP/1.1 200 OK\r\n
  snprintf(buf, sizeof buf, "HTTP/1.1 %d ", static_cast<int>(statusCode_));
  output->append(buf);
  output->append(statusMessage_);
  output->append("\r\n");

  // 2. 处理 Connection Header
  if (closeConnection_) {
    output->append("Connection: close\r\n");
  } else {
    output->append("Content-Length: ");
    snprintf(buf, sizeof buf, "%zu\r\n", body_.size());
    output->append(buf);
    output->append("Connection: Keep-Alive\r\n");
  }

  // 3. 写入其他 Headers
  for (const auto& header : headers_) {
    output->append(header.first);
    output->append(": ");
    output->append(header.second);
    output->append("\r\n");
  }

  // 4. 头部结束空行
  output->append("\r\n");

  // 5. 写入 Body
  output->append(body_);
}