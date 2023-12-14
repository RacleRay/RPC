# Hello world


### TCP buffer 
- 用于应用层数据的组装和拆解
- 使用 epoll 将 buffer 中数据异步发送出去
- 可以将多个包合并一起发送

> 对于 TCP 而言，它本身是字节流的传输，没有所谓的粘包，粘包是对与应用层数据包而言。 


### TCP client
非阻塞 connect 注意事项：
- 返回0，连接成功
- 返回-1，但是 errno==EINPROGRESS，连接正在建立，此时将 epoll 中去监听可写事件。当可写事件就绪，使用 getsockopt 获取 fd 上的错误，如果是 0 ，代表连接建立成功  


### 枚举类型反射
使用模板元编程，依靠 \__PRETTY_FUNCTIOIN__ 宏，实现枚举类型转字符串，以及字符串转枚举类型。


### RPC
controller: RPC 信息和配置
rpcchannel: 客户端与服务端通信


### TODO
- 使用协程优化代码中一些回调函数的功能