#include "InetAddress.hh"

#include <arpa/inet.h>

#include <array>
#include <cstring>
#include <stdexcept>

InetAddress::InetAddress(const std::string& ip, port_t port) {
  memset(&sock_in_, 0, sizeof(sock_in_));

  sock_in_.sin_family = AF_INET;
  sock_in_.sin_port = htons(port);
  if (::inet_pton(AF_INET, ip.c_str(), &sock_in_.sin_addr) <= 0) {
    throw std::invalid_argument("Invalid IPv4 address format: " + ip);
  }
}

std::string InetAddress::ToIp() const {
  std::array<char, INET_ADDRSTRLEN> buf{};
  ::inet_ntop(AF_INET, &sock_in_.sin_addr, buf.data(), buf.size());

  return {buf.data()};
}

std::string InetAddress::ToIpPort() const {
  return ToIp() + ":" + std::to_string(ToPort());
}

InetAddress::port_t InetAddress::ToPort() const {
  return ntohs(sock_in_.sin_port);
}