#pragma once
#include <sys/epoll.h>

#include <unordered_map>
#include <vector>

#include "Channel.hh"
#include "base/Macros.hh"
#include "base/Timestamp.hh"

class Epoll {
public:
  using ChannelList = std::vector<Channel*>;

  Epoll();
  DISALLOW_COPY_AND_MOVE(Epoll);
  ~Epoll();

  Timestamp poll(int timeoutMs, ChannelList* activeChannels);
  void UpdateChannel(Channel* channel);
  void RemoveChannel(Channel* channel);
  bool HasChannel(Channel* channel) const;

private:
  static constexpr int kInitEventListSize = 16;

  void Update(int operation, Channel* channel) const;
  void FillActiveChannels(int numEvents, ChannelList* activeChannels) const;

  fd_t epollfd_;
  std::vector<epoll_event> events_;
  std::unordered_map<int, Channel*> channels_;
};