#pragma once

#include <functional>

#include "netaddr.h"
#include "protocol/abstractproto.h"
#include "reactor/eventloop.h"
#include "tcpconnection.h"

namespace rayrpc {

class TcpClient {
  public:
    explicit TcpClient(NetAddr::s_ptr peer_addr);
    ~TcpClient();

    void connect(std::function<void()> done_cb);

    // 异步发送 message，如果发送成功，则回调 done_cb
    void writeMessage(AbstractProtocol::s_ptr protos, std::function<void(AbstractProtocol::s_ptr)> done_cb);

    // 异步读取 message，如果读取成功，则回调 done_cb
    void readMessage(const std::string& req_id, std::function<void(AbstractProtocol::s_ptr)> done_cb);

  private:
    NetAddr::s_ptr m_peer_addr;

    EventLoop *m_event_loop{nullptr};

    FdEvent *m_fd_event{nullptr};

    TcpConnection::s_ptr m_connection{nullptr};

    int m_fd{-1};
}; // class TcpClient

} // namespace rayrpc