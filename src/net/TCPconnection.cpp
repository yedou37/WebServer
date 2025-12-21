#include "TCPconnection.hh"

#include <unistd.h>

#include <functional>
#include <utility>

#include "Channel.hh"
#include "EventLoop.hh"
#include "Socket.hh"
#include "base/Macros.hh"

TCPConnection::TCPConnection(EventLoopBase* loop, std::string nameArg, fd_t sockfd,
                             const InetAddress& local_addr,  // NOLINT
                             const InetAddress& peer_addr)   // NOLINT
    : loop_(loop),
      name_(std::move(nameArg)),
      state_(State::kConnecting),
      socket_(std::make_unique<Socket>(sockfd)),
      channel_(std::make_unique<Channel>(loop, sockfd)),
      local_addr_(local_addr),
      peer_addr_(peer_addr) {
  socket_->setTcpNoDelay(true);
  // 给 Channel 设置回调函数
  // 当 Channel 收到 Poller 的通知时，会回调 TcpConnection 的方法
  channel_->SetReadCallback([this](Timestamp) { HandleRead(); });
  channel_->SetWriteCallback([this]() { HandleWrite(); });
  channel_->SetCloseCallback([this]() { HandleClose(); });
  channel_->SetErrorCallback([this]() { HandleError(); });
  socket_->setKeepAlive(true);
}

TCPConnection::~TCPConnection() = default;
void TCPConnection::ConnectEstablished() {
  assert(loop_->IsInEventLoopThread() == true);
  assert(state_ == State::kConnecting);
  setState(State::kConnected);
  channel_->Tie(shared_from_this());
  channel_->EnableRead();
  if (connection_callback_ != nullptr) {
    connection_callback_(shared_from_this());
  }
}
void TCPConnection::ConnectDestroyed() {
  assert(loop_->IsInEventLoopThread() == true);
  if (state_ == State::kConnected) {
    setState(State::kDisconnected);
    channel_->DisableAll();
    if (connection_callback_ != nullptr) {
      connection_callback_(shared_from_this());
    }
  }
  channel_->Remove();
}
void TCPConnection::HandleRead() {
  assert(loop_->IsInEventLoopThread() == true);
  int saved_errno{0};
  ssize_t n = input_buffer_.readFd(channel_->Getfd(), &saved_errno);
  if (n > 0) {
    // 读到了数据，调用用户的 OnMessage 回调
    // 用户需要在这里面解析 Buffer（解决粘包问题）
    if (message_callback_ != nullptr) {
      message_callback_(shared_from_this(), &input_buffer_, Timestamp::Now());  // TODO(yedou) timestamp
    }
  } else if (n == 0) {
    // 读到 0，表示对端关闭了连接
    HandleClose();
  } else {
    // 出错
    errno = saved_errno;
    HandleError();
  }
}

void TCPConnection::HandleWrite() {
  assert(loop_->IsInEventLoopThread() == true);
  if (channel_->IsWriting()) {
    ssize_t n = ::write(channel_->Getfd(), output_buffer_.peek(), output_buffer_.readableBytes());
    if (n > 0) {
      output_buffer_.retrieve(n);
      // 如果 output_buffer_ 中的数据全部发送完毕
      if (output_buffer_.readableBytes() == 0) {
        // 停止关注写事件，避免 busy-loop
        channel_->DisableWrite();

        // 如果有注册“写完成”回调，则调用
        if (write_complete_callback_ != nullptr) {
          loop_->QueueInLoop([this]() { write_complete_callback_(shared_from_this()); });
        }
        // 如果连接正处于“正在断开”状态，说明之前有一次 Shutdown() 调用
        // 因为缓冲区未发完而被延迟了。现在数据发完了，正是执行关闭的时刻。
        if (state_ == State::kDisconnecting) {
          shutdownInLoop();
        }
      }
    } else {
      // LOG_SYSERR << "TCPConnection::HandleWrite";
      perror("TCPConnection::HandleWrite");
    }
  } else {
    // LOG_TRACE << "Connection fd = " << channel_->Getfd() << " is down, no more writing";
    perror("TCPConnection::HandleWrite writing on non-writing channel");
  }
}

void TCPConnection::HandleClose() {
  assert(loop_->IsInEventLoopThread() == true);
  if (state_ == State::kDisconnected) {
    return;
  }
  assert(state_ == State::kConnected || state_ == State::kDisconnecting);
  setState(State::kDisconnected);
  channel_->DisableAll();
  std::shared_ptr<TCPConnection> guard(shared_from_this());
  if (connection_callback_ != nullptr) {
    connection_callback_(guard);
  }
  if (close_callback_ != nullptr) {
    close_callback_(guard);
  }
}

void TCPConnection::HandleError() {
  int err;
  socklen_t optlen = sizeof(err);
  if (::getsockopt(channel_->Getfd(), SOL_SOCKET, SO_ERROR, &err, &optlen) < 0) {
    err = errno;
  }
  // perror("TCPConnection::HandleError");
}

void TCPConnection::Send(const std::string& message) {
  if (state_ == State::kConnected) {
    if (loop_->IsInEventLoopThread()) {
      // 如果当前就在 I/O 线程，直接发送
      sendInLoop(message.c_str(), message.size());
    } else {
      // 如果在其他线程，将发送任务抛给 I/O 线程执行
      loop_->RunInLoop([this, message]() { this->sendInLoop(message.c_str(), message.size()); });
    }
  }
}
void TCPConnection::Send(Buffer* buf) {
  if (state_ == State::kConnected) {
    if (loop_->IsInEventLoopThread()) {
      sendInLoop(buf->peek(), buf->readableBytes());
    } else {
      std::string msg(buf->peek(), buf->readableBytes());
      buf->retrieveAll();
      loop_->RunInLoop([ptr = shared_from_this(), msg = std::move(msg)]() { ptr->sendInLoop(msg.data(), msg.size()); });
    }
  }
}
void TCPConnection::sendInLoop(const void* data, size_t len) {
  assert(loop_->IsInEventLoopThread());
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;

  // 1. 如果状态是断开连接，就不要发了
  if (state_ == State::kDisconnected) {
    perror("disconnected, give up writing");
    return;
  }

  // 2. 尝试直接发送数据
  // 条件：Channel 没有在监听写事件（说明缓冲区无积压）且 output_buffer_ 为空
  if (!channel_->IsWriting() && output_buffer_.readableBytes() == 0) {
    nwrote = ::write(channel_->Getfd(), data, len);
    if (nwrote >= 0) {
      remaining = len - nwrote;
      // 如果数据全部发送完毕，且用户注册了写完成回调，则触发它
      if (remaining == 0 && write_complete_callback_) {
        loop_->QueueInLoop([this]() { write_complete_callback_(shared_from_this()); });
      }
    } else {
      // write 出错
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        perror("TCPConnection::sendInLoop");
        if (errno == EPIPE || errno == ECONNRESET) {
          faultError = true;
        }
      }
    }
  }

  // 3. 如果还有剩余数据（说明 socket 缓冲区满了，或者刚才只写了一部分）
  //    且没有发生致命错误
  if (!faultError && remaining > 0) {
    // 把剩余数据存入 output_buffer_
    output_buffer_.append(static_cast<const char*>(data) + nwrote, remaining);
    // 4. 注册写事件
    // 这样当 socket 变得可写时，EventLoop 会回调 HandleWrite
    if (!channel_->IsWriting()) {
      channel_->EnableWrite();
    }
  }
}

void TCPConnection::Shutdown() {
  if (state_ == State::kConnected) {
    setState(State::kDisconnecting);
    loop_->RunInLoop([this]() { shutdownInLoop(); });
  }
}
void TCPConnection::shutdownInLoop() {
  assert(loop_->IsInEventLoopThread());
  if (!channel_->IsWriting()) {
    socket_->shutdownWrite();
  }
}
