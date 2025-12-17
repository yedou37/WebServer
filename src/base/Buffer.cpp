#include "Buffer.hh"

#include <sys/uio.h>
#include <unistd.h>

#include <array>
#include <cerrno>

namespace {
constexpr size_t kExtraBufSize = 65536;
}

ssize_t Buffer::readFd(int fd, int* savedErrno) {
  // 栈上的临时空间，64KB
  std::array<char, kExtraBufSize> extrabuf{};

  std::array<struct iovec, 2> vec{};
  const size_t writable = writableBytes();

  // 第一块缓冲区：Buffer 里的剩余空间
  vec[0].iov_base = begin() + writerIndex_;
  vec[0].iov_len = writable;
  // 第二块缓冲区：栈上的临时空间
  vec[1].iov_base = extrabuf.data();
  vec[1].iov_len = extrabuf.size();

  // 如果 Buffer 剩余空间够大（>= 64KB），就不需要第二块了
  const int iovcnt = (writable < extrabuf.size()) ? 2 : 1;

  const ssize_t n = ::readv(fd, vec.data(), iovcnt);

  if (n < 0) {
    *savedErrno = errno;
  } else if (static_cast<size_t>(n) <= writable) {
    // 读取的数据量小于 Buffer 剩余空间，直接移动 writerIndex
    writerIndex_ += n;
  } else {
    // 读取的数据量超过了 Buffer 剩余空间，也就是部分数据在 extrabuf 里
    writerIndex_ = buffer_.size();          // Buffer 满了
    append(extrabuf.data(), n - writable);  // 把 extrabuf 里的数据追加进 Buffer（会自动扩容）
  }

  return n;
}