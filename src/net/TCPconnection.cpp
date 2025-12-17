#include "TCPconnection.hh"

#include <unistd.h>

#include <functional>
#include <utility>

#include "Channel.hh"
#include "EventLoop.hh"
#include "Socket.hh"
#include "base/Macros.hh"

TCPConnection::TCPConnection(EventLoop* loop, std::string nameArg, fd_t sockfd,
                             const InetAddress& local_addr,  // NOLINT
                             const InetAddress& peer_addr)   // NOLINT
    : loop_(loop),
      name_(std::move(nameArg)),
      state_(State::kConnecting),
      socket_(std::make_unique<Socket>(sockfd)),
      channel_(std::make_unique<Channel>(loop, sockfd)),
      local_addr_(local_addr),
      peer_addr_(peer_addr) {
  // 给 Channel 设置回调函数
  // 当 Channel 收到 Poller 的通知时，会回调 TcpConnection 的方法
  channel_->SetReadCallback([this](Timestamp) { HandleRead(); });
  channel_->SetWriteCallback([this]() { HandleWrite(); });
  channel_->SetCloseCallback([this]() { HandleClose(); });
  channel_->SetErrorCallback([this]() { HandleError(); });
  socket_->setKeepAlive(true);
}

TCPConnection::~TCPConnection() {
  channel_->DisableAll();
  channel_->Remove();
}

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
      if (output_buffer_.readableBytes() == 0) {
        channel_->DisableWrite();
        if (write_complete_callback_ != nullptr) {
          loop_->QueueInLoop([this]() { write_complete_callback_(shared_from_this()); });
        }
        if (state_ == State::kDisconnecting) {
          shutdownInLoop();
        }
      }
    } else {
      perror("TCPConnection::HandleWrite");
    }
  } else {
    perror("TCPConnection::HandleWrite");
  }
}

void TCPConnection::HandleClose() {
  assert(loop_->IsInEventLoopThread() == true);
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
