#pragma once

#include <memory>

#include "iothread/iothread.h"
#include "tcp/netaddr.h"
#include "tcp/tcpbuffer.h"


namespace rayrpc {
    
enum TcpState {
    NotConnected = 1,
    Connected,
    HalfConnected,
    Closed
};

class TcpConnection {
public:
    using s_ptr = std::shared_ptr<TcpConnection>;

    TcpConnection(IOThread* io_thread, int fd, int buffer_size, NetAddr::s_ptr peer_addr);
    ~TcpConnection();

    void onRead();

    void onWrite();

    void excute();

    void setState(const TcpState& state) noexcept;

    TcpState getState() const noexcept;

    void clear();

    void shutdown();  // 服务端主动关闭连接

private:
    IOThread* m_io_thread{nullptr};

    NetAddr::s_ptr m_local_addr;
    NetAddr::s_ptr m_peer_addr;

    TcpBuffer::s_ptr m_in_buffer;
    TcpBuffer::s_ptr m_out_buffer;

    FdEvent* m_fd_event{nullptr};

    TcpState m_state;

    int m_fd{0};  // connnection fd
};  // class TcpConnection

}  // namespace rayrpc