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

    m_clear_client_timer_event = std::make_shared<TimerEvent>(10000, true, [this]() {clearClientTimerCallback();});
    m_main_event_loop->addTimerEvent(m_clear_client_timer_event);

    INFOLOG("TcpServer::TcpServer : TcpServer listen on [%s].", m_local_addr->toString().c_str());
}

TcpServer::~TcpServer() {
    // deal with raw pointer
    if (m_main_event_loop) {
        delete m_main_event_loop;
        m_main_event_loop = nullptr;
    }
    if (m_io_thread_group) {
        delete m_io_thread_group;
        m_io_thread_group = nullptr;
    }
    if (m_listen_fd_event) {
        delete m_listen_fd_event;
        m_listen_fd_event = nullptr;
    }
}

void TcpServer::onAccept() {
    auto accept_ret = m_acceptor->accept();
    int clientfd = accept_ret.first;
    NetAddr::s_ptr client_addr = accept_ret.second;
    m_client_count++;

    IOThread* io_thread = m_io_thread_group->getIOThread();
    int buf_size = 128;
    TcpConnection::s_ptr tcp_conn = std::make_shared<TcpConnection>(io_thread->getEventLoop(), clientfd, buf_size, client_addr, m_local_addr);

    tcp_conn->setState(TcpState::Connected);

    m_clients.insert(tcp_conn);

    INFOLOG("TcpServer::onAccept : TcpServer accept client fd [%d].", clientfd);
}


void TcpServer::start() {
    m_io_thread_group->start();
    m_main_event_loop->loop();
}


void TcpServer::clearClientTimerCallback() {
    auto it = m_clients.begin();
    for (it = m_clients.begin(); it != m_clients.end();) {
        if ((*it) != nullptr && (*it).use_count() > 0 && (*it)->getState() == TcpState::Closed) {
            DEBUGLOG("TcpServer::clearClientTimerCallback : TcpServer clear client fd [%d].", (*it)->getConnFd());
            it = m_clients.erase(it);
        } else {
            it++;
        }
    }
}


}  // namespace rayrpc