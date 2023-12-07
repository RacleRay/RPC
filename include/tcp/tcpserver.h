#pragma once

#include <set>

#include "iothread/iothreadgroup.h"
#include "reactor/eventloop.h"
#include "tcp/netaddr.h"
#include "tcp/tcpacceptor.h"
#include "tcp/tcpconnection.h"


namespace rayrpc {

class TcpServer {
  public:
    explicit TcpServer(NetAddr::s_ptr local_addr, size_t nsubreactor = 1);
    ~TcpServer();

    void start();

  private:
    void init();

    void onAccept(); // 有新连接时callback

  private:
    TcpAcceptor::s_ptr m_acceptor;

    NetAddr::s_ptr m_local_addr;

    EventLoop *m_main_event_loop{nullptr}; // main reactor

    IOThreadGroup *m_io_thread_group{nullptr}; // sub reactor

    FdEvent *m_listen_fd_event;

    std::set<TcpConnection::s_ptr> m_clients;

    int m_client_count{0};
    size_t m_n_subreactor{1};
}; // class TcpServer

} // namespace rayrpc