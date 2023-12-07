#include <unistd.h>

#include "log.h"
#include "reactor/fdevent.h"
#include "reactor/fdeventgroup.h"
#include "reflect_enum.h"
#include "tcp/tcpconnection.h"


namespace rayrpc {

// fd : connection fd
TcpConnection::TcpConnection(IOThread* io_thread, int fd, int buffer_size, NetAddr::s_ptr peer_addr)
    : m_io_thread(io_thread), m_fd(fd), m_peer_addr(peer_addr), m_state(TcpState::NotConnected)
{
    m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
    m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

    // fd managed by global group class
    m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(fd);
    
    m_fd_event->setNonBlock();
    m_fd_event->listen(FdEvent::TriggerEvent::IN_EVENT, [this]() { onRead(); });
    
    io_thread->getEventLoop()->addEpollEvent(m_fd_event);
}

TcpConnection::~TcpConnection() {
    DEBUGLOG("TcpConnection::~TcpConnection");
}

// read from connection socket to in_buffer
void TcpConnection::onRead() {
    DEBUGLOG("TcpConnection::onRead : Tcp state is [%s].", refenum::get_enum_name(m_state).c_str());

    if (m_state != TcpState::Connected) {
        ERRLOG("TcpConnection::onRead : state error, client is not connneted. addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
        return;
    }

    // 虽然是 LT 模式，但是每次有可读事件，还是读完 socket 缓冲区中的内容
    bool is_read_all = false;
    bool is_close = false;
    while (!is_read_all) {
        if (m_in_buffer->writeAble() == 0) {
            m_in_buffer->resizeBuffer(m_in_buffer->getBufferSize() * 2);
        }

        size_t read_count = m_in_buffer->writeAble();
        size_t write_index = m_in_buffer->writeIndex();

        ssize_t ret = read(m_fd, &(m_in_buffer->m_buffer[write_index]), read_count);
        DEBUGLOG("TcpConnection::onRead : success read %d bytes from addr[%s], client fd[%d]", ret, m_peer_addr->toString().c_str(), m_fd);

        // 检查是否读完
        if (ret > 0) {
            m_in_buffer->moveWriteIndex(ret);
            if (ret == read_count) {
                continue; 
            }
            if (ret < read_count) {
                is_read_all = true;
                break;
            }
        } else if (ret == 0) {
            is_close = true;
            break;
        } else if (ret == -1 && errno == EAGAIN) {
            is_read_all = true;
            break;
        }
    }

    if (is_close) {
        INFOLOG("TcpConnection::onRead : client is closed, peer addr [%s], clientfd [%d]", m_peer_addr->toString().c_str(), m_fd);
        clear();
        return;
    }

    if (!is_read_all) {
        ERRLOG("TcpConnection::onRead : not read all data.");
    }

    excute();
}

void TcpConnection::onWrite() {
    DEBUGLOG("TcpConnection::onWrite : Tcp state is [%s].", refenum::get_enum_name(m_state).c_str());

    if (m_state != TcpState::Connected) {
        ERRLOG("TcpConnection::onWrite : state error, client is not connneted. addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
        return;
    }

    bool is_write_all = false;
    while (!is_write_all) {
        if (m_out_buffer->readAble() == 0) {
            DEBUGLOG("TcpConnection::onWrite : no data to write to client [%s].", m_peer_addr->toString().c_str());
            is_write_all = true;
            break;
        }

        size_t write_size = m_out_buffer->readAble();
        size_t read_index = m_out_buffer->readIndex();

        ssize_t ret = write(m_fd, &(m_out_buffer->m_buffer[read_index]), write_size);
        if (ret >= write_size) {
            DEBUGLOG("TcpConnection::onwrite : no data to write to client [%s].", m_peer_addr->toString().c_str());
            is_write_all = true;
            break;
        }
        if (ret == -1 && errno == EAGAIN) {
            ERRLOG("TcpConnection::onwrite : write to client [%s] error, out buffer is full.", m_peer_addr->toString().c_str());
            break;
        }
    }

    if (is_write_all) {
        // 移除可写事件
        m_fd_event->cancel(FdEvent::TriggerEvent::OUT_EVENT);
        m_io_thread->getEventLoop()->addEpollEvent(m_fd_event);
    }
}

// 解析 RPC ，并执行请求，再把 RPC 处理结果发送出去
void TcpConnection::excute() {
    std::vector<char> tmp;

    size_t content_size = m_in_buffer->readAble();
    tmp.resize(content_size);

    m_in_buffer->readFromBuffer(tmp, content_size);

    std::string msg(tmp.begin(), tmp.end());

    INFOLOG("TcpConnection::excute : msg from client [%s], msg size [%d]", m_peer_addr->toString().c_str(), msg.size());

    // echo for test
    m_out_buffer->writeToBuffer(msg.c_str(), msg.length());

    // 添加可写事件
    m_fd_event->listen(FdEvent::TriggerEvent::OUT_EVENT, [this]() { onWrite(); });
    m_io_thread->getEventLoop()->addEpollEvent(m_fd_event);
}

void TcpConnection::setState(const TcpState& state) noexcept {
    m_state = state;
}

TcpState TcpConnection::getState() const noexcept {
    return m_state;
}

void TcpConnection::clear() {
    if (m_state == TcpState::Closed) {
        return;
    }
    m_fd_event->cancel(FdEvent::TriggerEvent::IN_EVENT);
    m_fd_event->cancel(FdEvent::TriggerEvent::OUT_EVENT);

    m_io_thread->getEventLoop()->deleteEpollEvent(m_fd_event);
    m_state = TcpState::Closed;
}

// 服务端主动关闭连接
void TcpConnection::shutdown() {
    DEBUGLOG("TcpConnection::shutdown : Tcp state is [%s].", refenum::get_enum_name(m_state).c_str());
    if (m_state == TcpState::Closed || m_state == TcpState::NotConnected) {
        return;
    }

    m_state = TcpState::HalfConnected;
    DEBUGLOG("TcpConnection::shutdown : Tcp state is [%s].", refenum::get_enum_name(m_state).c_str());

    // shutdown 后，服务器程序不会再对这个 fd 进行写操作了，发送 FIN 报文， 触发了四次挥手的第一个阶段
    // 此后，当 fd 发生可读事件，但是可读的数据为0，即 对端也发送了 FIN
    ::shutdown(m_fd, SHUT_RDWR);
}

}  // namespace rayrpc