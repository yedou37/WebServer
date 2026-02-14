#include "Epoll.hh"

#include <unistd.h>  // for close

#include <cassert>
#include <cstring>
#include <iostream>  // for perror / logging

#include "base/Macros.hh"

Epoll::Epoll() : epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize) {
  if (epollfd_ < 0) {
    perror("epoll_create1() failed");
    exit(1);
  }
}

Epoll::~Epoll() {
  ::close(epollfd_);
}

Timestamp Epoll::poll(int timeoutMs, ChannelList* activeChannels) {
  int numEvents = ::epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);

  int savedErrno = errno;
  Timestamp now = Timestamp::Now();

  if (numEvents > 0) {
    FillActiveChannels(numEvents, activeChannels);

    if (numEvents == static_cast<int>(events_.size())) {
      events_.resize(events_.size() * 2);
    }
  } else if (numEvents == 0) {
    // Timeout
  } else {
    if (savedErrno != EINTR) {
      errno = savedErrno;
      perror("Epoll::poll() error");
    }
  }
  return now;
}

void Epoll::FillActiveChannels(int numEvents, ChannelList* activeChannels) const {
  for (int i = 0; i < numEvents; ++i) {
    auto* channel = static_cast<Channel*>(events_[i].data.ptr);
    channel->SetRevents(events_[i].events);
    activeChannels->push_back(channel);
  }
}

void Epoll::UpdateChannel(Channel* channel) {
  const Channel::State state = channel->GetState();
  const fd_t fd = channel->Getfd();

  if (state == Channel::State::kNew || state == Channel::State::kDeleted) {
    if (state == Channel::State::kNew) {
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    } else {
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }

    channel->SetState(Channel::State::kAdded);
    Update(EPOLL_CTL_ADD, channel);
  } else {
    // state == Channel::State::kAdded
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);

    if (channel->IsNoneEvent()) {
      Update(EPOLL_CTL_DEL, channel);
      channel->SetState(Channel::State::kDeleted);
    } else {
      Update(EPOLL_CTL_MOD, channel);
    }
  }
}

void Epoll::RemoveChannel(Channel* channel) {
  int fd = channel->Getfd();
  assert(channel->IsNoneEvent());
  if (channels_.contains(fd)) {
    size_t n = channels_.erase(fd);
    (void)n;  // 防止 Release 模式下未使用变量的警告
    assert(n == 1);
  }
  if (channel->GetState() == Channel::State::kAdded) {
    Update(EPOLL_CTL_DEL, channel);
  }
  channel->SetState(Channel::State::kNew);
}

void Epoll::Update(int operation, Channel* channel) const {
  epoll_event event = {};
  event.events = channel->GetEvents();
  event.data.ptr = channel;

  if (::epoll_ctl(epollfd_, operation, channel->Getfd(), &event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      // 在 removeChannel 中我们经常会遇到 DEL 失败的情况（比如对端早已关闭），
      // LOG_ERROR << "epoll_ctl op=" << operation << " fd=" << channel->Getfd();
    } else {
      // LOG_FATAL << "epoll_ctl op=" << operation << " fd=" << channel->Getfd();
      perror("epoll_ctl error");
    }
  }
}

bool Epoll::HasChannel(Channel* channel) const {
  auto it = channels_.find(channel->Getfd());
  return it != channels_.end() && it->second == channel;
}