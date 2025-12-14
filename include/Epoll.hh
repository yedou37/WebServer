#pragma once
#include <sys/epoll.h>

#include <unordered_map>
#include <vector>

#include "Channel.hh"
#include "base/Macros.hh"
#include "base/Timestamp.hh"
class Epoll {
public:
  using epoll_event_t = struct epoll_event;
  using ChannelList = std::vector<Channel*>;
  using epoll_operation_t = int;
  DISALLOW_COPY_AND_MOVE(Epoll);
  Epoll();
  ~Epoll();
  // 核心接口 1：调用 epoll_wait
  // 将活跃的 channel 填入 activeChannels
  Timestamp poll(const int& timeoutMs, ChannelList* activeChannels);

  // 核心接口 2：调用 epoll_ctl (ADD/MOD/DEL)
  void UpdateChannel(Channel* channel);

  // 核心接口 3：从 map 中移除
  void RemoveChannel(Channel* channel);

  // 辅助接口：判断 channel 是否在当前 poller 中
  bool HasChannel(Channel* channel) const;

private:
  void Update(epoll_operation_t operation, Channel* channel) const;
  void FillActiveChannels(const int& numEvents, ChannelList* activeChannels) const;
  static constexpr int kInitEventListSize{16};
  const fd_t epollfd_;
  std::vector<epoll_event_t> events_;
  std::unordered_map<fd_t, Channel*> channels_;
};