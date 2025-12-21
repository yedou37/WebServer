#pragma once

#include <cstdint>
#include <string>
#include <string_view>  // 必须包含
#include <unordered_map>
#include <utility>

#include "Buffer.hh"

class HttpRequest {
public:
  using field_t = std::string;
  using value_t = std::string;
  using headers_t = std::unordered_map<field_t, value_t>;

  enum class Method : std::uint8_t { INVALID, GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH, TRACE };
  enum class Version : std::uint8_t { HTTP_1_0, HTTP_1_1, UNKNOWN };

  HttpRequest() = default;
  ~HttpRequest() = default;

  void SetPath(const char* start, const char* end) { path_.assign(start, end); }
  void SetQuery(const char* start, const char* end) { query_.assign(start, end); }
  void SetVersion(Version v) { version_ = v; }
  void SetBody(std::string body) { body_ = std::move(body); }

  [[nodiscard]] bool SetMethod(const char* start, const char* end);

  void AddHeader(std::string_view key, std::string_view value) {
    headers_.insert_or_assign(std::string(key), std::string(value));
  };

  void AddHeader(const char* start, const char* colon, const char* valStart, const char* valEnd) {  // NOLINT
    // 这里不可避免要有一次构造，因为要存入 map
    std::string key(start, colon);
    std::string val(valStart, valEnd);
    headers_[std::move(key)] = std::move(val);
  }

  [[nodiscard]] const value_t& GetHeader(const field_t& field) const;

  [[nodiscard]] Method GetMethod() const { return method_; }
  [[nodiscard]] Version GetVersion() const { return version_; }
  [[nodiscard]] const headers_t& GetHeaders() const { return headers_; }
  [[nodiscard]] const std::string& path() const { return path_; }
  [[nodiscard]] const std::string& query() const { return query_; }
  [[nodiscard]] const std::string& body() const { return body_; }

  void reset() {
    method_ = Method::INVALID;
    version_ = Version::UNKNOWN;
    path_.clear();
    query_.clear();
    headers_.clear();
    body_.clear();
  }

  // Swap 保持不变 ...
  void swap(HttpRequest& other) noexcept {
    using std::swap;
    swap(method_, other.method_);
    swap(version_, other.version_);
    swap(path_, other.path_);
    swap(query_, other.query_);
    swap(headers_, other.headers_);
    swap(body_, other.body_);
  }

private:
  Method method_{Method::INVALID};
  Version version_{Version::UNKNOWN};
  std::string path_;
  std::string query_;
  headers_t headers_;
  std::string body_;
};