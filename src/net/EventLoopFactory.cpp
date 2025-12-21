#include "EventLoopFactory.hh"

#include "EpollEventLoop.hh"
#include "IOUringEventLoop.hh"

std::unique_ptr<EventLoopBase> EventLoopFactory::Create(EventLoopType type) {
#ifdef USE_IO_URING
  if (type == EventLoopType::IO_URING) {
    return std::make_unique<IOUringEventLoop>();
  }
#endif
  
  // Default to epoll
  return std::make_unique<EpollEventLoop>();
}
