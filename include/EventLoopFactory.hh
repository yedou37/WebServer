#pragma once

#include <memory>

#include "EpollEventLoop.hh"
#include "EventLoopBase.hh"
#include "IOUringEventLoop.hh"
enum class EventLoopType : std::uint8_t { EPOLL, IO_URING };

class EventLoopFactory {
public:
  static std::unique_ptr<EventLoopBase> Create(EventLoopType type = EventLoopType::EPOLL);
};