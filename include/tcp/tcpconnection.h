#pragma once

#include <memory>


#include "iothread/iothread.h"
#include "protocol/abstractcoder.h"
#include "protocol/abstractproto.h"
#include "tcp/netaddr.h"
#include "tcp/tcpbuffer.h"


namespace rayrpc {
    
enum TcpState {
    NotConnected = 1,
    Connected,
    HalfConnected,
    Closed
};

enum TcpConnectionType {
    TcpConnectionAtServer = 1,  // 在服务端，代表对客户端的连接
    TcpConnectionAtClient = 2,  // 在客户端，代表对服务端的连接
};

class TcpConnection {
public:
    using s_ptr = std::shared_ptr<TcpConnection>;

    // 不使用 IOThread 是因为，在写到 TcpClient 时，发现没有 IOThread，只有一个 EventLoop.
    // TcpConnection(IOThread* io_thread, int fd, int buffer_size, NetAddr::s_ptr peer_addr);
    TcpConnection(EventLoop* event_loop, int fd, int buffer_size, NetAddr::s_ptr peer_addr, TcpConnectionType type = TcpConnectionAtServer);
    ~TcpConnection();

    void onRead();

    void onWrite();

    void excute();

    void setState(const TcpState& state) noexcept;

    TcpState getState() const noexcept;

    void clear();

    void shutdown();  // 服务端主动关闭连接

    void setConnectionType(TcpConnectionType type);

    void listenWritable();

    void listenReadable();

    void pushSendMessage(AbstractProtocol::s_ptr protocol, std::function<void(AbstractProtocol::s_ptr)> callback);

    void pushRecvMessage(const std::string& req_id, std::function<void(AbstractProtocol::s_ptr)> callback);;

private:
    // IOThread* m_io_thread{nullptr};
    EventLoop* m_event_loop{nullptr};

    NetAddr::s_ptr m_local_addr;
    NetAddr::s_ptr m_peer_addr;

    TcpBuffer::s_ptr m_in_buffer;
    TcpBuffer::s_ptr m_out_buffer;

    FdEvent* m_fd_event{nullptr};

    TcpState m_state;

    int m_fd{0};  // connnection fd

    TcpConnectionType m_connection_type{TcpConnectionAtServer};

    AbstractCoder* m_coder{nullptr};

    // protocol : write callback(protocol)
    std::vector<std::pair<AbstractProtocol::s_ptr, std::function<void(AbstractProtocol::s_ptr)>>> m_write_callbacks;

    // req_id : read callback
    std::map<std::string, std::function<void(AbstractProtocol::s_ptr)>> m_read_callbacks;

};  // class TcpConnection

}  // namespace rayrpc