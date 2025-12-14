#include "Epoll.hh"

#include <cassert>
#include <cstring>
#include <stdexcept>

#include "base/Timestamp.hh"

static constexpr int kNew = -1;
static constexpr int kAdded = 1;
static constexpr int kDeleted = 2;

Timestamp Epoll::poll(const int& timeoutMs, ChannelList* activeChannels) {
  int numEvents = ::epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);

  int savedErrno = errno;  // 立刻保存 errno，防止被 Timestamp::Now 覆盖
  Timestamp now = Timestamp::Now();

  if (numEvents > 0) {
    // LOG_INFO << numEvents << " events happened";
    FillActiveChannels(numEvents, activeChannels);

    // 如果 activeChannels 满了，说明并发很高，需要扩容
    // 因为epoll_wait函数是向一个c风格的数组中写入数据，所以需要手动扩容
    if (numEvents == static_cast<int>(events_.size())) {
      events_.resize(events_.size() * 2);
    }
  } else if (numEvents == 0) {
    // LOG_TRACE << "nothing happened"; // 超时
  } else {
    // 错误处理
    if (savedErrno != EINTR) {
      // errno = EINTR 意味着被信号打断，不是真正的错误，可以忽略
      // LOG_ERROR << "Epoll::poll() error";
      perror("Epoll::poll() error");
    }
  }
  return now;
}

void Epoll::FillActiveChannels(const int& numEvents, ChannelList* activeChannels) const {
  for (int i = 0; i < numEvents; ++i) {
    // void* -> Channel*
    auto* channel = static_cast<Channel*>(events_[i].data.ptr);
    channel->SetRevents(events_[i].events);
    activeChannels->emplace_back(channel);
  }
}
void Epoll::UpdateChannel(Channel* channel) {
  const int index = channel->GetIndex();
  epoll_operation_t operation;
  fd_t fd = channel->Getfd();
  if (index == kNew || index == kDeleted) {
    operation = EPOLL_CTL_ADD;
    if (index == kNew) {
      channels_.insert_or_assign(fd, channel);
    } else {
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }
    channel->SetIndex(kAdded);
    Update(operation, channel);
  } else {
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(index == kAdded);
    if (channel->IsNoneEvent()) {
      operation = EPOLL_CTL_DEL;
      Update(operation, channel);
      channel->SetIndex(kDeleted);
    } else {
      operation = EPOLL_CTL_MOD;
      Update(operation, channel);
    }
  }
}

void Epoll::RemoveChannel(Channel* channel) {
  fd_t fd = channel->Getfd();
  assert(channels_.find(fd) != channels_.end());
  assert(channels_[fd] == channel);
  assert(channel->IsNoneEvent());

  auto n = channels_.erase(fd);
  assert(n == 1);
  if (channel->GetIndex() == kAdded) {
    Update(EPOLL_CTL_DEL, channel);
  }
  channel->SetIndex(kNew);
}

Epoll::Epoll() : epollfd_(epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize) {
  if (epollfd_ < 0) {
    perror("epoll_create1() failed");
    throw std::runtime_error("epoll_create1() failed: " + std::string(strerror(errno)));
  }
}

Epoll::~Epoll() {
  ::close(epollfd_);
}

void Epoll::Update(epoll_operation_t operation, Channel* channel) const {
  epoll_event event;
  memset(&event, 0, sizeof(event));
  event.events = channel->GetEvents();
  event.data.ptr = channel;
  if (::epoll_ctl(epollfd_, operation, channel->Getfd(), &event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      perror("epoll_ctl() failed when EPOLL_CTL_DEL");
    } else {
      perror("epoll_ctl() failed");
    }
  }
}