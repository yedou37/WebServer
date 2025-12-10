#include <unistd.h>  // for read, write, close

#include <array>
#include <cstring>  // for memset
#include <iostream>

#include "InetAddress.hh"
#include "Socket.hh"

void test_server() {
  std::cout << "[DEBUG] Starting test server...\n";

  try {
    // 1. Create server address object (127.0.0.1 : 8080)
    constexpr InetAddress::port_t port = 8080;
    InetAddress localAddr("127.0.0.1", port);
    std::cout << "[DEBUG] Address created: " << localAddr.ToIpPort() << '\n';

    // 2. Create listening Socket
    Socket listenSocket;

    // 3. Set options (reuse address and port to prevent 'Address already in use' on restart)
    listenSocket.setReuseAddr(true);
    listenSocket.setReusePort(true);

    // 4. Bind address
    listenSocket.bindAddress(localAddr);
    std::cout << "[DEBUG] Bind success.\n";

    // 5. Start listening
    listenSocket.listen();
    std::cout << "[DEBUG] Listening on " << localAddr.ToIpPort() << "...\n";

    // 6. Accept connection (blocking operation until client connects)
    InetAddress clientAddr;
    Socket connSocket = listenSocket.accept(&clientAddr);

    if (connSocket.fd() >= 0) {
      std::cout << "[DEBUG] Connection accepted!\n";
      std::cout << "[DEBUG] Client Address: " << clientAddr.ToIpPort() << '\n';

      // 7. Simple read/write test
      constexpr size_t BUFFER_SIZE = 1024;
      std::array<char, BUFFER_SIZE> buf{};

      // Read data
      ssize_t n = connSocket.read(buf.data(), buf.size());
      if (n > 0) {
        std::cout << "[DEBUG] Received " << n << " bytes: " << buf.data() << '\n';

        std::string msg = "Hello from your WebServer Socket wrapper!\n";
        connSocket.write(msg.c_str(), msg.size());
      } else {
        std::cout << "[DEBUG] Client disconnected or read error.\n";
      }

    } else {
      std::cerr << "[ERROR] Accept failed.\n";
    }

  } catch (const std::exception& e) {
    std::cerr << "[EXCEPTION] " << e.what() << '\n';
  }
}

int main() {
  test_server();
  return 0;
}