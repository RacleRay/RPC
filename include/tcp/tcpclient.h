#pragma once

#include <functional>

#include "netaddr.h"
#include "protocol/abstractproto.h"
#include "reactor/eventloop.h"
#include "tcpconnection.h"

namespace rayrpc {

class TcpClient {
  public:
    using s_ptr = std::shared_ptr<TcpClient>;

    explicit TcpClient(NetAddr::s_ptr peer_addr);
    ~TcpClient();

    void connect(std::function<void()> done_cb);

    // 异步发送 message，如果发送成功，则回调 done_cb
    void writeMessage(AbstractProtocol::s_ptr protos, std::function<void(AbstractProtocol::s_ptr)> done_cb);

    // 异步读取 message，如果读取成功，则回调 done_cb
    void readMessage(const std::string& req_id, std::function<void(AbstractProtocol::s_ptr)> done_cb);

    void stop();  // 由于使用了服务端 eventloop，设置 client 主动终止循环的选项，防止多次连接后，资源不释放

    int getConnectErrorCode() const;

    std::string getConnectErrorInfo() const;

    NetAddr::s_ptr getPeerAddr() const;

    NetAddr::s_ptr getLocalAddr() const;

    void initLocalAddr();

  private:
    NetAddr::s_ptr m_peer_addr;
    NetAddr::s_ptr m_local_addr;

    EventLoop *m_event_loop{nullptr};

    FdEvent *m_fd_event{nullptr};

    TcpConnection::s_ptr m_connection{nullptr};

    int m_fd{-1};

    int m_connection_err_code{0};
    std::string m_connection_err_info;
}; // class TcpClient

} // namespace rayrpc