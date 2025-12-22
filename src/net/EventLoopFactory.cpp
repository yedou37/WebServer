#include "EventLoopFactory.hh"

#include "EpollEventLoop.hh"
#include "IOUringEventLoop.hh"

std::unique_ptr<EventLoopBase> EventLoopFactory::Create(EventLoopType type) {
  if (type == EventLoopType::IO_URING) {
    return std::make_unique<IOUringEventLoop>();
  }

  // Default to epoll
  return std::make_unique<EpollEventLoop>();
}
