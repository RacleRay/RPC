#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

#include "errorcode.h"
#include "log.h"
#include "reactor/fdeventgroup.h"
#include "tcp/netaddr.h"
#include "tcp/tcpclient.h"


namespace rayrpc {

TcpClient::TcpClient(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr)  {
    m_event_loop = EventLoop::GetCurrentEventLoop();

    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd < 0) {
        ERRLOG("TcpClient::TcpClient : failed to create socket fd");
        return;
    }

    m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(m_fd);
    m_fd_event->setNonBlock();

    m_connection = std::make_shared<TcpConnection>(m_event_loop, m_fd, 128, peer_addr);
    m_connection->setConnectionType(TcpConnectionAtClient);
}

TcpClient::~TcpClient() {
    DEBUGLOG("TcpClient::~TcpClient");
    if (m_fd > 0) {
        close(m_fd);
    }
}

void TcpClient::connect(std::function<void()> done_cb) {
    // Nonblock connect
    int ret = ::connect(m_fd, m_peer_addr->getSockAddr(), m_peer_addr->getSockAddrLen());
    // EOF
    if (ret == 0) {
        DEBUGLOG("TcpClient::connect : connect success, server [%s]", m_peer_addr->toString().c_str());
        
        m_connection->setState(TcpState::Connected);
        initLocalAddr();

        if (done_cb) {
            done_cb();
        }
    }
    else if (ret < 0) {
        // 连接中
        if (errno == EINPROGRESS) {
            // 监听 OUT 事件，如果非阻塞 connect fd 可写，则回调 done_cb
            m_fd_event->listen(FdEvent::TriggerEvent::OUT_EVENT, 
                [this, done_cb]() 
                {
                    bool is_connected = false;
                    int error = 0;
                    socklen_t errlen = sizeof(error);

                    int ret = getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &error, &errlen);
                    if (error == 0) {
                        DEBUGLOG("TcpClient::connect : connect success, server [%s]", m_peer_addr->toString().c_str());
                        initLocalAddr();
                        is_connected = true;
                        m_connection->setState(TcpState::Connected);
                    }
                    else if (error == ECONNREFUSED) {
                        m_connection_err_code = ERROR_PEER_CLOSED;
                        m_connection_err_info = "connection refused by server, sys error = " + std::string(strerror(errno));
                    }
                    else {
                        m_connection_err_code = ERROR_FAILED_CONNECT;
                        m_connection_err_info = "connect unknown error, sys error = " + std::string(strerror(errno));
                    }

                    if (!is_connected) {
                        ERRLOG("TcpClient::connect : connect failed, server [%s] error [%d], [%s]", m_peer_addr->toString().c_str(), error, strerror(error));
                        // close(m_fd);
                        // m_fd = ::socket(m_peer_addr->getFamily(), SOCK_STREAM, 0);
                    }

                    // TODO: 也许需要删除 m_fd_event ，防止重复触发
                    // LT 模式，需要删除可写事件
                    m_fd_event->cancel(FdEvent::TriggerEvent::OUT_EVENT);
                    m_event_loop->addEpollEvent(m_fd_event);

                    if (is_connected && done_cb) {
                        done_cb();
                    }
                });
            m_event_loop->addEpollEvent(m_fd_event);

            if (!m_event_loop->isLooping()) {
                m_event_loop->loop();
            }
        }
        // 连接出错
        else {
            ERRLOG("TcpClient::connect : connect failed, server [%s] error [%d], [%s]", m_peer_addr->toString().c_str(), errno, strerror(errno));
            m_connection_err_code = ERROR_FAILED_CONNECT;
            m_connection_err_info = "connect unknown error, sys error = " + std::string(strerror(errno));
        }
    }
}

// 异步发送 message，如果发送成功，则回调 done_cb
void TcpClient::writeMessage(AbstractProtocol::s_ptr protos, std::function<void(AbstractProtocol::s_ptr)> done_cb) {
    // 注册write成功的回调
    m_connection->pushSendMessage(std::move(protos), std::move(done_cb));
    m_connection->listenWritable();
}

// 异步读取 message，如果读取成功，则回调 done_cb
void TcpClient::readMessage(const std::string& req_id, std::function<void(AbstractProtocol::s_ptr)> done_cb) {
    // 注册recv成功的回调
    m_connection->pushRecvMessage(req_id, std::move(done_cb));
    m_connection->listenReadable();
}

void TcpClient::stop() {
    if (m_event_loop->isLooping()) {
        m_event_loop->stop();
    }
}


void TcpClient::initLocalAddr() {
    sockaddr_in local_addr;
    socklen_t len = sizeof(local_addr);

    int ret = getsockname(m_fd, reinterpret_cast<sockaddr*>(&local_addr), &len);
    if (ret != 0) {
        ERRLOG("TcpClient::initLocalAddr : failed to get local addr, errno=%d, error=%s", errno, strerror(errno));
        return;
    }

    m_local_addr = std::make_shared<IPNetAddr>(local_addr);
}


int TcpClient::getConnectErrorCode() const {
    return m_connection_err_code;
}

std::string TcpClient::getConnectErrorInfo() const {
     return m_connection_err_info;
}

NetAddr::s_ptr TcpClient::getPeerAddr() const {
     return m_peer_addr;
}

NetAddr::s_ptr TcpClient::getLocalAddr() const {
     return m_local_addr;
}

void TcpClient::addTimerEvent(TimerEvent::s_ptr timer_event) {
    m_event_loop->addTimerEvent(std::move(timer_event));
}


}  // namespace rayrpc