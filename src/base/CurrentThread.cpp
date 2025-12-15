#include "CurrentThread.hh"

#include <sys/syscall.h>
#include <unistd.h>

namespace CurrentThread {
thread_local int t_cachedTid = 0;
void cacheTid() {
  if (t_cachedTid == 0) {
    // 通过系统调用获取当前线程的真实 TID
    t_cachedTid = static_cast<int>(::syscall(SYS_gettid));
  }
}
}  // namespace CurrentThread