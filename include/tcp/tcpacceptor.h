#pragma once

#include "tcp/netaddr.h"

namespace rayrpc {

class TcpAcceptor {
  public:
    using s_ptr = std::shared_ptr<TcpAcceptor>;

    explicit TcpAcceptor(NetAddr::s_ptr local_addr);
    ~TcpAcceptor() = default;

    std::pair<int, NetAddr::s_ptr> accept();

    int getListenFd() const noexcept;

  private:
    NetAddr::s_ptr m_local_addr; // 服务端监听地址  ip:port

    int m_family{-1};
    int m_listenfd{-1};
}; // class TcpAcceptor

} // namespace rayrpc