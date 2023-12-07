#pragma once

#include "tcp/netaddr.h"

namespace rayrpc {
    
class TcpAcceptor {
public:
    explicit TcpAcceptor(NetAddr::s_ptr local_addr);
    ~TcpAcceptor() = default;

    int accept();

    int getListenFd() const noexcept;

private:
    NetAddr::s_ptr m_local_addr;   // 服务端监听地址  ip:port

    int m_family{-1};
    int m_listenfd{-1};
};  // class TcpAcceptor

}  // namespace rayrpc