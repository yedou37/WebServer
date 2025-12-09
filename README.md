# Janus WebServer

<div align="center">

![C++](https://img.shields.io/badge/language-C++-blue.svg)

</div>

## Project Introduction

Janus WebServer is a high-performance web server built with modern C++ and designed with Reactor pattern and event-driven architecture. The project aims to provide an efficient, stable, and extensible network server framework.

The project is currently under development and implementing features step by step according to a detailed roadmap.

## Features

- 🚀 epoll-based IO multiplexing
- ⚡ Event-driven Reactor pattern
- 🧵 Multi-threading support (One Loop Per Thread)
- 📦 RAII resource management mechanism
- 🌐 HTTP protocol support
- 🔧 High-performance buffer design
- 🛠 Modern C++ features

## Architecture Overview

```
┌─────────────────┐
│   HttpServer    │ Application Layer: HTTP protocol parsing and handling
└─────────────────┘
┌─────────────────┐
│   TcpServer     │ Transport Layer: TCP connection management
└─────────────────┘
┌─────────────────┐
│  EventLoopPool  │ Threading Model: Thread pool management
└─────────────────┘
┌─────────────────┐
│   EventLoop     │ Core Layer: Event loop
└─────────────────┘
┌─────────────────┐
│   Epoll         │ System Layer: IO multiplexing
└─────────────────┘
┌─────────────────┐
│ Socket/Channel  │ Foundation Layer: Socket encapsulation
└─────────────────┘
```

## Development Roadmap

### Phase 1: Network Programming Foundation (Socket RAII)

Implement basic socket encapsulation and build the foundation of RAII resource management model.

### Phase 2: Reactor Core (Epoll & Loop)

Build the core components of event-driven architecture, including Channel, Epoll and EventLoop.

### Phase 3: Connection Management and Buffer (TcpConnection)

Implement object-oriented connection management and introduce Buffer to solve packet sticking problems.

### Phase 4: HTTP Protocol Layer (Application Layer)

Add HTTP protocol parsing capabilities to build a complete HTTP server.

### Phase 5: High Concurrency Thread Pool (Multi-threading)

Implement multi-threading model to improve concurrent processing capability of the server.

### Phase 6: Optimization and Enhancement

Performance optimization and implementation of advanced features.

See detailed roadmap in [ROADMAP.md](ROADMAP.md)

## Quick Start

### Requirements

- C++20 or higher
- Linux system (epoll support)
- CMake 3.10+

### Build Instructions

```bash
# Clone the project
git clone ssh://git@ssh.github.com:443/yedou37/WebServer.git
cd janus-webserver

# Create build directory
mkdir build && cd build

# Compile the project
cmake ..
make
```

### Run the Server

```bash
./janus-webserver
```

By default, the server listens on port 8080.

## Project Structure

```
src/
├── base/              # Base components
│   ├── Macros.hh      # Common macro definitions
│   └── Buffer.cpp     # Data buffer
├── net/               # Network modules
│   ├── Socket.cpp     # Socket encapsulation
│   ├── InetAddress.cpp # IP address encapsulation
│   ├── Channel.cpp    # Event channel
│   ├── Epoll.cpp      # epoll operation encapsulation
│   ├── EventLoop.cpp  # Event loop
│   ├── EventLoopThread.cpp       # Event loop thread
│   ├── EventLoopThreadPool.cpp   # Thread pool
│   ├── TcpConnection.cpp         # TCP connection
│   └── TcpServer.cpp             # TCP server
├── http/              # HTTP module (planned)
└── main.cpp           # Main program entry
```

## Contribution Guidelines

Issues and Pull Requests are welcome to help improve the project. Before contributing code, please ensure:

1. Follow the existing code style of the project
2. Add appropriate test cases
3. Update relevant documentation

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- Inspired by the muduo network library
- Referenced design ideas from CS144 Computer Networking course

---

_Janus WebServer - Building high-performance web servers with modern C++_
