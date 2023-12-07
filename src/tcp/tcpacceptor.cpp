#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>

#include "log.h"
#include "tcp/netaddr.h"
#include "tcp/tcpacceptor.h"


namespace rayrpc {

TcpAcceptor::TcpAcceptor(NetAddr::s_ptr local_addr) : m_local_addr(local_addr) {
    if (!local_addr->checkValid()) {
        ERRLOG("TcpAcceptor::TcpAcceptor : invalid local addr %s", local_addr->toString().c_str());
        std::abort();
    }

    m_family = m_local_addr->getFamily();

    m_listenfd = socket(m_family, SOCK_STREAM, 0);
    if (m_listenfd < 0) {
        ERRLOG("TcpAcceptor::TcpAcceptor : listen socket, invalid listenfd %d", m_listenfd);
        std::abort();
    }

    int val = 1;
    if (setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != 0) {
        ERRLOG("TcpAcceptor::TcpAcceptor : setsockopt REUSEADDR error, errno=%d, error=%s", errno, strerror(errno));
    }

    socklen_t len = m_local_addr->getSockLen();
    if (bind(m_listenfd, m_local_addr->getSockAddr(), len) != 0) {
        ERRLOG("TcpAcceptor::TcpAcceptor : bind error, errno=%d, error=%s", errno, strerror(errno));
        std::abort();
    }

    if (listen(m_listenfd, 1000) != 0) {
        ERRLOG("TcpAcceptor::TcpAcceptor : listen error, errno=%d, error=%s", errno, strerror(errno));
        std::abort();
    }
}


std::pair<int, NetAddr::s_ptr> TcpAcceptor::accept() {
    if (m_family == AF_INET) {
        sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t clien_addr_len = sizeof(clien_addr_len);

        int client_fd = ::accept(m_listenfd, reinterpret_cast<sockaddr *>(&client_addr), &clien_addr_len);
        if (client_fd < 0) { 
            ERRLOG("TcpAcceptor::accept : accept error, errno=%d, error=%s", errno, strerror(errno)); }

        // IPNetAddr peer_addr(client_addr);
        IPNetAddr::s_ptr peer_addr = std::make_shared<IPNetAddr>(client_addr);
        INFOLOG("TcpAcceptor::accept : A client have accpeted succ, peer addr [%s]", peer_addr->toString().c_str());
        
        return {client_fd, peer_addr};
    } else {
        // other protocal
        // TODO: not implemented
        return {-1, nullptr};
    }
}


int TcpAcceptor::getListenFd() const noexcept {
    return m_listenfd;
}

} // namespace rayrpc