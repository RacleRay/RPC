#include "log.h"
#include "reactor/eventloop.h"
#include "tcp/tcpconnection.h"
#include "tcp/tcpserver.h"


namespace rayrpc {
    
TcpServer::TcpServer(NetAddr::s_ptr local_addr, size_t nsubreactor)
    : m_local_addr(local_addr), m_n_subreactor(nsubreactor) 
{
    m_acceptor = std::make_shared<TcpAcceptor>(local_addr);

    m_main_event_loop = EventLoop::GetCurrentEventLoop();
    m_io_thread_group = new IOThreadGroup(m_n_subreactor);

    m_listen_fd_event = new FdEvent(m_acceptor->getListenFd());
    m_listen_fd_event->listen(FdEvent::TriggerEvent::IN_EVENT, [this](){onAccept();});

    m_main_event_loop->addEpollEvent(m_listen_fd_event);

    INFOLOG("TcpServer::TcpServer : TcpServer listen on [%s].", m_local_addr->toString().c_str());
}

TcpServer::~TcpServer() {
    if (m_main_event_loop) {
        delete m_main_event_loop;
        m_main_event_loop = nullptr;
    }
}

void TcpServer::onAccept() {
    auto accept_ret = m_acceptor->accept();
    int clientfd = accept_ret.first;
    NetAddr::s_ptr client_addr = accept_ret.second;
    m_client_count++;

    // TOOD: add clientfd to IO thread (subreactor).
    IOThread* io_thread = m_io_thread_group->getIOThread();
    int buf_size = 128;
    TcpConnection::s_ptr tcp_conn = std::make_shared<TcpConnection>(io_thread->getEventLoop(), clientfd, buf_size, client_addr);

    tcp_conn->setState(TcpState::Connected);

    m_clients.insert(tcp_conn);

    INFOLOG("TcpServer::onAccept : TcpServer accept client fd [%d].", clientfd);
}


void TcpServer::start() {
    m_io_thread_group->start();
    m_main_event_loop->loop();
}

}  // namespace rayrpc