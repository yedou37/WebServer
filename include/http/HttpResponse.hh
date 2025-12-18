#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include "Buffer.hh"

class HttpResponse {
public:
  enum class HttpStatusCode : std::uint16_t {
    k200Ok = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,

  };

  explicit HttpResponse(bool close) : closeConnection_(close) {}

  void SetStatusCode(HttpStatusCode code) { statusCode_ = code; }
  void SetStatusMessage(std::string message) { statusMessage_ = std::move(message); }
  void SetCloseConnection(bool on) { closeConnection_ = on; }
  void SetBody(std::string body) { body_ = std::move(body); }
  void SetContentType(const std::string& contentType) { AddHeader("Content-Type", contentType); }

  void AddHeader(const std::string& key, const std::string& value) { headers_[key] = value; }

  void AppendToBuffer(Buffer* outputBuffer) const;
  [[nodiscard]] bool CloseConnection() const { return closeConnection_; }

private:
  std::unordered_map<std::string, std::string> headers_;
  HttpStatusCode statusCode_{HttpStatusCode::k200Ok};
  std::string statusMessage_{"OK"};
  bool closeConnection_;  // 是否在发送后断开连接 (Connection: close)
  std::string body_;
};