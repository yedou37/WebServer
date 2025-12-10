#pragma once

#include <netinet/in.h>
#include <sys/socket.h>

#include <string>

class InetAddress {
public:
  using port_t = uint16_t;
  using sin_t = struct sockaddr_in;
  InetAddress() = default;
  InetAddress(const std::string& ip, port_t port);
  explicit InetAddress(const sin_t& sock_in) : sock_in_(sock_in) {};

  [[nodiscard]] const sockaddr* GetSocketAddr() const { return reinterpret_cast<const sockaddr*>(&sock_in_); }

  [[nodiscard]] socklen_t GetSocketAddrLen() const { return sizeof(sock_in_); }

  [[nodiscard]] std::string ToIp() const;
  [[nodiscard]] std::string ToIpPort() const;
  [[nodiscard]] port_t ToPort() const;

private:
  sin_t sock_in_;
};