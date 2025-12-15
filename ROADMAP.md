# 🗺️ Janus WebServer 开发路线图 (Roadmap)

## 📦 阶段一：网络编程地基 (Socket RAII)

**目标**：不使用 Epoll，仅使用封装好的类实现一个同步阻塞的 Echo Server。
**重点**：RAII 资源管理，避免手动 close 描述符。

- [x] **Infrastructure**: 确保 `Macros.hh` 中的 `DISALLOW_COPY_AND_MOVE` 可用。
- [x] **InetAddress**:
  - [x] 实现 IP/Port 的转换（`sockaddr_in`）。
  - [x] 实现 `toIp()`, `toPort()`, `toIpPort()`。
- [x] **Socket**:
  - [x] 构造函数创建 socket (`AF_INET`, `SOCK_STREAM`, `0`)。
  - [x] 析构函数 `close(fd)`。
  - [x] 封装 `bind()`, `listen()`, `accept()`。
  - [x] 封装 `setReuseAddr()`, `setNonBlock()`。
- [x] **验证 (v0.1)**:
  - 在 `main.cpp` 中使用 `Socket` 和 `InetAddress` 写一个死循环接受连接并 `read/write`。
  - 测试：`telnet localhost 8080`。

## ⚡ 阶段二：Reactor 核心 (Epoll & Loop)

**目标**：实现“事件驱动”模型。单线程下能同时处理多个连接的事件。

- [x] **Channel (核心)**:
  - [x] 封装 `fd` 和感兴趣的事件 (`EPOLLIN`, `EPOLLOUT`)。
  - [x] 设置回调函数 (`std::function`): `readCallback`, `writeCallback`。
  - [x] 实现 `handleEvent()`: 根据 `revents` 调用不同的回调。
- [x] **Epoll (Poller)**:
  - [x] 封装 `epoll_create1`。
  - [x] 封装 `updateChannel` (`epoll_ctl`): 负责 add/mod/del。
  - [x] 封装 `poll` (`epoll_wait`): 返回活跃的 Channels 列表。
- [ ] **EventLoop (驱动)**:
  - [ ] 持有 `Epoll` 对象。
  - [ ] 主循环 `loop()`: 调用 `epoll->poll()` 获取活跃 Channel，然后调用 `channel->handleEvent()`。
- [ ] **验证 (v0.2)**:
  - 在 `main.cpp` 中创建一个 `EventLoop`。
  - 把 `listenFd` 封装成 Channel，注册读事件。
  - 当有新连接时，打印 "New Connection"。

## 🔗 阶段三：连接管理与缓冲区 (TcpConnection)

**目标**：面向对象地管理连接。引入 Buffer 解决粘包问题（CS144 核心应用）。

- [ ] **Buffer**:
  - [ ] 模仿 CS144 ByteStream 或 Muduo Buffer。
  - [ ] `std::vector<char>` + `readIndex` + `writeIndex`。
  - [ ] 实现 `append()`, `retrieve()`, `retrieveAllAsString()`。
- [ ] **Acceptor (可选，建议放在 TCPConnection 之前)**:
  - [ ] 专门处理 `listenFd` 的读事件（即 `accept` 新连接）。
- [ ] **TcpConnection**:
  - [ ] **拥有**：`Socket` (管理 fd 生命周期), `Channel` (管理事件), `inputBuffer`, `outputBuffer`。
  - [ ] **handleRead()**: 从 socket 读数据 -> inputBuffer -> 触发用户回调。
  - [ ] **handleWrite()**: outputBuffer -> socket。如果没写完，关注 `EPOLLOUT` 等待下次写。
- [ ] **TcpServer**:
  - [ ] 管理所有 `TcpConnection` (`std::map<int, TcpConnectionPtr>`)。
  - [ ] 处理新连接建立和连接断开。
- [ ] **验证 (v0.3)**:
  - 写一个基于 `TcpServer` 的 Echo Server。
  - 客户端发什么，服务端回什么（非阻塞）。

## 🌐 阶段四：HTTP 协议层 (Application Layer)

**目标**：解析 HTTP 文本，构建 HTTP 响应。

- [ ] **HttpRequest**:
  - [ ] 存储 Method, URL, Version, Headers, Body。
- [ ] **HttpResponse**:
  - [ ] 存储 StatusCode, Headers, Body。
  - [ ] 提供 `appendBuffer` 将响应序列化进 Buffer。
- [ ] **HttpContext (状态机)**:
  - [ ] 状态定义：`ExpectRequestLine`, `ExpectHeaders`, `ExpectBody`。
  - [ ] 解析 `inputBuffer`，填充 `HttpRequest` 对象。
- [ ] **HttpServer**:
  - [ ] 继承或持有 `TcpServer`。
  - [ ] 设置 `TcpServer` 的 `onMessage` 回调：数据到达 -> HttpContext 解析 -> 构造 HttpResponse -> 发送。
- [ ] **验证 (v0.4)**:
  - 浏览器访问 `http://localhost:8080`，显示 "Hello World" 或你的静态页面。

## 🧵 阶段五：高并发线程池 (Multi-threading)

**目标**：One Loop Per Thread，充分利用多核 CPU。

- [ ] **EventLoopThread**:
  - [ ] 启动一个新线程 (`std::thread`)，在里面运行 `EventLoop::loop()`。
- [ ] **EventLoopThreadPool**:
  - [ ] 管理 N 个 `EventLoopThread`。
  - [ ] `getNextLoop()`: 使用 Round-Robin 算法选择下一个 Loop。
- [ ] **跨线程通信 (难点)**:
  - [ ] 在 `EventLoop` 中加入 `wakeupFd` (`eventfd`)。
  - [ ] 实现 `runInLoop()` 和 `queueInLoop()`：允许主线程把任务（如“处理新连接”）塞给子线程去执行。
- [ ] **集成**:
  - [ ] `TcpServer` 启动时开启线程池。
  - [ ] Accept 到的新连接分配给子 Loop 管理。
- [ ] **验证 (v0.5)**:
  - 使用 `wrk` 或 `Webbench` 进行压测，观察 QPS 和 CPU 使用率（应能跑满多核）。

## 🛠️ 阶段六：优化与完善 (Optimization)

- [ ] **Timer (定时器)**:
  - [ ] 使用 `timerfd` 或 `priority_queue`。
  - [ ] 清理空闲连接（踢掉超时不发数据的客户端）。
- [ ] **Async Logging (异步日志)**:
  - [ ] 双缓冲技术：前端写 LogBuffer，后端刷盘。
- [ ] **单元测试**:
  - [ ] 完善 `tests/` 目录下的测试用例。

---
