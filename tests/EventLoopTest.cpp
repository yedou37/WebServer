#include <cstdio>
#include <thread>

#include "EventLoop.hh"

EventLoop* g_loop;

void threadFunc() {
  printf("子线程: 等待5秒...\n");
  sleep(5);  // NOLINT

  // 子线程调用主线程 loop 的 RunInLoop
  g_loop->RunInLoop([]() {
    printf("主线程: 我被唤醒了！这是通过 eventfd 触发的！\n");
    g_loop->Quit();  // 唤醒后让主循环退出
  });
}

int main() {
  EventLoop loop;
  g_loop = &loop;

  std::thread t(threadFunc);

  printf("主线程: 开始 Loop，现在会阻塞...\n");
  loop.Loop();  // 如果 EventLoop 写得对，这里会阻塞，直到子线程唤醒它

  printf("主线程: Loop 结束。\n");
  t.join();
  return 0;
}