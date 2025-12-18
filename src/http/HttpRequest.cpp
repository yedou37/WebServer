#include "http/HttpRequest.hh"

#include <string_view>

bool HttpRequest::SetMethod(const char* start, const char* end) {
  std::string_view m(start, end - start);
  if (m == "GET") {
    method_ = Method::GET;
  } else if (m == "POST") {
    method_ = Method::POST;
  } else if (m == "PUT") {
    method_ = Method::PUT;
  } else if (m == "DELETE") {
    method_ = Method::DELETE;
  } else if (m == "HEAD") {
    method_ = Method::HEAD;
  } else if (m == "OPTIONS") {
    method_ = Method::OPTIONS;
  } else if (m == "PATCH") {
    method_ = Method::PATCH;
  } else if (m == "TRACE") {
    method_ = Method::TRACE;
  } else {
    method_ = Method::INVALID;
  }
  return method_ != Method::INVALID;
}

const HttpRequest::value_t& HttpRequest::GetHeader(const field_t& field) const {
  // 声明一个静态的空字符串，用于找不到时返回
  // 必须是 static，否则返回局部变量的引用会崩溃
  static const std::string empty_string;

  auto it = headers_.find(field);
  if (it != headers_.end()) {
    return it->second;  // 返回 map 内部 value 的引用
  }
  return empty_string;
}