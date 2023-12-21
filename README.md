# Hello world

> 依赖环境：
>
> - protobuf
> - tinyxml

### RPC

主从 Reactor 设计，结合线程池的小型 RPC。服务端是异步的，客户端的 RPC 调用需要同步等待，没有对客户端的实现优化，只用作测试。服务端的异步，是通过注册回调实现，整个程序的 connect、read、write 逻辑是和回调任务异步进行的。

处理流程：

1. 客户端从 Stub 类调用接口，由 RpcChannel 发起向服务端的连接。
2. Epoll in 事件触发回调，从 in buffer 读取并解析自定义协议数据，取得 service name 和 method name。
3. 拿到客户端连接后，从线程池选择一个线程，负责新连接建立的 TcpConnection 事件处理，事件触发的任务全都以回调函数的方式，注册到每个子线程中 EventLoop 的任务队列中。通过 Wakeup fd 设计，在主 reactor 向从 reactor 添加事件任务时，及时唤醒 epoll_wait 等待的从 reactor 事件循环。
4. 服务端从 Stub 接口中搜索对应的 RPC 服务，执行服务。Service 基类提供接口，具体业务处理由子类实现，并由 Service 的 CallMethod 成员函数，调用子类中 method name 对应的方法。
5. 按协议中 response 的格式，构造返回消息包。由 Epoll out 事件触发回调，发送消息包，并回收本次调用中的资源消耗。



- rpc controller: RPC 运行时信息和配置
- rpc channel: 客户端与服务端通信支持。比如，其中设计了 RPC 超时处理，向事件循环注册回调逻辑，判断是否已经正常处理，如果不是，则返回超时消息。但是超时的处理，有点棘手，可以查询服务状态，可以重试（需要保证服务调用接口的幂等性）。
- rpc interface: 作为用户自定义的回调函数类，在业务执行完毕后，执行其中注册的功能函数
- rpc closure: RPC 调用结束时，执行的回调函数类，在 Service CallMethod 处传入，并在 rpc interface 中通过基类的 RUN 接口调用执行。



### 枚举类型反射

使用模板元编程，依靠 \__PRETTY_FUNCTIOIN__ 宏，实现枚举类型转字符串，以及字符串转枚举类型。



### Simple Double Buffered Log

使用简单的异步落盘日志。



### 自定义基于 Protobuf 的协议格式

目的是便捷地区分不同地数据包，并记录辅助信息，方便系统分析。当然，此例程只是简单地实现了一种可行的协议。



### TCP buffer 

- 用于应用层数据的组装和拆解
- 使用 epoll 将 buffer 中数据异步发送出去
- 可以将多个包合并一起发送

> 对于 TCP 而言，它本身是字节流的传输，没有所谓的粘包，粘包是对与应用层数据包而言。 




### TCP client
非阻塞 connect 注意事项：
- 返回0，连接成功
- 返回-1，但是 errno==EINPROGRESS，连接正在建立，此时将 epoll 中去监听可写事件。当可写事件就绪，使用 getsockopt 获取 fd 上的错误，如果是 0 ，代表连接建立成功  




### TODO
- 使用协程优化代码中一些回调函数的功能