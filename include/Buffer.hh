#pragma once

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer {
public:
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;

  explicit Buffer(size_t initialSize = kInitialSize)
      : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend) {}

  // 可读字节数
  [[nodiscard]] size_t readableBytes() const { return writerIndex_ - readerIndex_; }
  // 可写字节数
  [[nodiscard]] size_t writableBytes() const { return buffer_.size() - writerIndex_; }
  // 头部预留空间（方便加包头长度）
  [[nodiscard]] size_t prependableBytes() const { return readerIndex_; }

  // 返回可读数据的指针
  [[nodiscard]] const char* peek() const { return begin() + readerIndex_; }

  // 取出 len 长度的数据（移动 readerIndex）
  void retrieve(size_t len) {
    assert(len <= readableBytes());
    if (len < readableBytes()) {
      readerIndex_ += len;
    } else {
      retrieveAll();
    }
  }

  void retrieveAll() {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }

  // 把 buffer 转成 string 返回
  std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); }

  std::string retrieveAsString(size_t len) {
    assert(len <= readableBytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;
  }

  // 写入数据
  void append(const std::string& str) { append(str.data(), str.size()); }

  void append(const char* data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
  }

  // 确保有足够的写空间，不够就扩容
  void ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
      makeSpace(len);
    }
    assert(writableBytes() >= len);
  }

  char* beginWrite() { return begin() + writerIndex_; }
  [[nodiscard]] const char* beginWrite() const { return begin() + writerIndex_; }

  ssize_t readFd(int fd, int* savedErrno);

private:
  char* begin() { return &*buffer_.begin(); }
  [[nodiscard]] const char* begin() const { return &*buffer_.begin(); }

  void makeSpace(size_t len) {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
      // 真的不够了，resize
      buffer_.resize(writerIndex_ + len);
    } else {
      // 内部腾挪，把数据移到最前面
      assert(kCheapPrepend < readerIndex_);
      size_t readable = readableBytes();
      std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
    }
  }

  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;
};
