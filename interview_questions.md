# Reactor模式网络库面试题汇总

## 1. EventLoop类相关问题

### 问题1: EventLoop中的pending functors机制是如何工作的？解决了什么问题？
- **代码位置**: `include/EventLoop.hh` (L36-L38), `src/net/EventLoop.cpp` (L100-L126)
- **原因**: 这个机制允许其他线程向EventLoop线程投递任务，解决了跨线程操作的问题
- **参考答案**: 
  - EventLoop内部维护了一个pendingFunctors_容器，当外部线程需要在EventLoop线程执行任务时，会将任务放入这个容器
  - 通过mutex保护多线程访问的安全性
  - Wakeup机制确保EventLoop不会阻塞在epoll_wait上，及时执行pending任务

### 问题2: EventLoop中为什么要使用wakeupFd_？它是如何工作的？
- **代码位置**: `include/EventLoop.hh` (L42-L44), `src/net/EventLoop.cpp` (L14-L30)
- **原因**: 解决了在多线程环境下唤醒EventLoop的问题
- **参考答案**:
  - 使用eventfd创建一个专门的唤醒fd
  - 将wakeupFd_加入到epoll监控中
  - 当其他线程需要唤醒EventLoop时，向wakeupFd_写入数据，触发读事件，从而打断epoll_wait

### 问题3: EventLoop的线程安全性是如何保证的？
- **代码位置**: `include/EventLoop.hh` (L54-L55), `src/net/EventLoop.cpp` (L100-L126)
- **原因**: EventLoop需要支持跨线程的任务投递
- **参考答案**:
  - 通过IsInEventLoopThread()检查确保单线程执行
  - 使用mutex保护pendingFunctors_的并发访问
  - Wakeup机制确保任务及时执行

## 2. Channel类相关问题

### 问题4: Channel类中为什么要使用weak_ptr进行tie操作？
- **代码位置**: `include/Channel.hh` (L73-L75), `src/net/Channel.cpp` (L30-L44)
- **原因**: 防止对象生命周期管理问题
- **参考答案**:
  - 当Channel执行回调时，可能底层对象（如TcpConnection）已被销毁
  - 使用weak_ptr可以检测对象是否还存在
  - 避免悬空指针导致的崩溃

### 问题5: Channel的状态管理机制是怎样的？
- **代码位置**: `include/Channel.hh` (L21-L25), `src/net/Channel.cpp` (L47-L74)
- **原因**: 状态管理是Channel正确操作的基础
- **参考答案**:
  - kNew: 新创建的Channel，尚未添加到Poller中
  - kAdded: 已添加到Poller中
  - kDeleted: 已从Poller中删除
  - 通过状态管理确保操作顺序正确，防止重复添加或删除

### 问题6: Channel如何处理各种事件？
- **代码位置**: `src/net/Channel.cpp` (L47-L74)
- **原因**: Channel是事件处理的核心组件
- **参考答案**:
  - 分别处理EPOLLHUP、EPOLLERR、EPOLLIN/EPOLLPRI/EPOLLRDHUP、EPOLLOUT事件
  - 通过不同的回调函数处理不同类型事件
  - 优先处理错误和关闭事件

## 3. TCPConnection类相关问题

### 问题7: TCPConnection中状态转换的时机和原因是什么？
- **代码位置**: `include/TCPconnection.hh` (L62), `src/net/TCPconnection.cpp` (L31-L53)
- **原因**: 状态管理是连接管理的基础
- **参考答案**:
  - kConnecting: 连接正在建立
  - kConnected: 连接已建立
  - kDisconnecting: 连接正在断开
  - kDisconnected: 连接已断开
  - 不同状态下允许的操作不同，防止非法操作

### 问题8: TCPConnection如何处理TCP粘包和拆包问题？
- **代码位置**: `src/net/TCPconnection.cpp` (L55-L75), `include/TCPconnection.hh` (L67-L68)
- **原因**: TCP是流协议，需要处理数据边界问题
- **参考答案**:
  - 使用input_buffer_接收数据
  - 将完整的应用层消息交给上层处理
  - 上层协议负责解析Buffer中的数据，解决粘包问题

### 问题9: TCPConnection的发送机制是如何设计的？
- **代码位置**: `src/net/TCPconnection.cpp` (L134-L194)
- **原因**: 发送机制直接影响性能和可靠性
- **参考答案**:
  - 优先尝试直接发送
  - 如果socket缓冲区满，则将数据暂存到output_buffer_
  - 当socket变得可写时，通过HandleWrite发送缓冲区中的数据
  - 这种机制保证了数据的可靠发送

## 4. Buffer类相关问题

### 问题10: Buffer的内存布局是怎样的？为什么要这样设计？
- **代码位置**: `include/Buffer.hh` (L14-L24), `include/Buffer.hh` (L31-L33)
- **原因**: 合理的内存布局能提高性能
- **参考答案**:
  - 预留kCheapPrepend字节在头部，方便添加包头
  - readable区域存储有效数据
  - writable区域表示可写空间
  - 这种设计支持高效的读写操作

### 问题11: Buffer的readFd方法是如何实现零拷贝优化的？
- **代码位置**: `src/base/Buffer.cpp` (L14-L43)
- **原因**: 零拷贝优化能显著提升性能
- **参考答案**:
  - 使用readv系统调用，支持分散读
  - 一次调用可以从多个缓冲区读取数据
  - 避免了多次系统调用的开销

### 问题12: Buffer的动态扩容机制是怎样的？
- **代码位置**: `include/Buffer.hh` (L72-L78), `include/Buffer.hh` (L85-L105)
- **原因**: 动态扩容保证了灵活性
- **参考答案**:
  - 先尝试内部腾挪，将数据移到前面
  - 如果还是不够，再进行真正的扩容
  - 扩容策略是按需分配，避免浪费内存

## 5. Epoll/Poller相关问题

### 问题13: Epoll类的UpdateChannel方法是如何工作的？
- **代码位置**: `src/net/Epoll.cpp` (L56-L85)
- **原因**: 这是epoll操作的核心逻辑
- **参考答案**:
  - 根据Channel的不同状态执行不同的epoll_ctl操作
  - kNew或kDeleted状态时执行EPOLL_CTL_ADD
  - kAdded状态时根据事件类型执行EPOLL_CTL_MOD或EPOLL_CTL_DEL
  - 确保epoll状态与Channel状态一致

### 问题14: Epoll的事件处理流程是怎样的？
- **代码位置**: `src/net/Epoll.cpp` (L26-L48)
- **原因**: 理解事件处理流程对掌握整个Reactor模式很重要
- **参考答案**:
  - 调用epoll_wait等待事件
  - 将就绪事件填充到activeChannels列表
  - 如果事件列表满了，会自动扩容
  - 返回当前时间戳用于后续处理

## 6. 多线程模型相关问题

### 问题15: EventLoopThreadPool的负载均衡策略是怎样的？
- **代码位置**: `src/net/EventLoopThreadPool.cpp` (L20-L26)
- **原因**: 负载均衡直接影响多线程性能
- **参考答案**:
  - 使用轮询(Round Robin)策略分发连接
  - 通过next_索引依次选择EventLoop
  - 简单有效，实现复杂度低

### 问题16: 为什么使用One Loop Per Thread模型？
- **代码位置**: `include/EventLoop.hh` (L30), `include/EventLoopThread.hh`
- **原因**: 这是Reactor模式的关键设计
- **参考答案**:
  - 避免了锁的竞争，提高了性能
  - 每个线程拥有自己的事件循环，简化了线程安全问题
  - 符合Reactor模式的设计思想

## 7. 设计模式相关问题

### 问题17: 项目中应用了哪些设计模式？
- **代码位置**: 多个文件
- **原因**: 设计模式体现了架构的优雅性
- **参考答案**:
  - Reactor模式: 事件驱动，分发处理
  - Observer模式: Channel观察fd事件变化
  - RAII: 资源管理
  - 工厂模式: 连接创建

### 问题18: 为什么在Channel中使用回调函数而不是继承？
- **代码位置**: `include/Channel.hh` (L30-L33)
- **原因**: 这涉及到扩展性的设计
- **参考答案**:
  - 回调函数更加灵活，避免了复杂的继承体系
  - 支持lambda表达式等现代C++特性
  - 降低了组件间的耦合度

## 8. 对比性问题

### 问题19: 为什么选择epoll而不是select/poll？
- **代码位置**: `include/Epoll.hh`
- **原因**: 这是高性能服务器的关键选择
- **参考答案**:
  - epoll效率更高，不受FD_SETSIZE限制
  - epoll是事件驱动的，不需要遍历所有fd
  - 适合大量并发连接的场景

### 问题20: 这种Reactor设计的优缺点是什么？
- **代码位置**: 多个文件
- **原因**: 理解设计权衡
- **参考答案**:
  - 优点：高效、可扩展、适合高并发
  - 缺点：编程复杂度高、不适合计算密集型任务

## 9. 拓展性问题

### 问题21: 如何改进当前的TCPConnection以支持SSL？
- **原因**: SSL是现代网络服务的必需功能
- **参考答案**:
  - 添加SSL上下文和连接状态
  - 封装SSL读写操作
  - 修改发送接收流程以支持加密解密

### 问题22: 如何优化高并发下的性能表现？
- **原因**: 高并发是这类服务器的关键指标
- **参考答案**:
  - 优化锁粒度，减少竞争
  - 实现连接池复用
  - 使用更高效的内存池管理
  - 优化网络参数和内核参数

### 问题23: 如果要支持UDP协议，需要做哪些改动？
- **原因**: UDP适用于实时性要求高的场景
- **参考答案**:
  - 创建UDPConnection类
  - 修改事件处理逻辑以适应UDP特点
  - 不需要连接建立/断开流程
  - 考虑广播和多播需求

