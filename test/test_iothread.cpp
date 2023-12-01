#include <arpa/inet.h>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "iothread/iothread.h"
#include "log.h"
#include "reactor/eventloop.h"
#include "reactor/fdevent.h"
#include "timer/timerevent.h"


void test_io_thread() {
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        ERRLOG("listenfd = -1");
        std::abort();
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_port = htons(12310);
    addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &addr.sin_addr);

    int rt = bind(listenfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    if (rt != 0) {
        ERRLOG("bind error");
        std::abort();
    }

    rt = listen(listenfd, 100);
    if (rt != 0) {
        ERRLOG("listen error");
        std::abort();
    }

    rayrpc::FdEvent event(listenfd);
    event.listen(rayrpc::FdEvent::TriggerEvent::IN_EVENT, [listenfd]() {
        sockaddr_in peer_addr;
        socklen_t addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, sizeof(peer_addr));
        int clientfd = accept(listenfd, reinterpret_cast<sockaddr *>(&peer_addr), &addr_len);
        DEBUGLOG(
            "success get client fd[%d], peer addr: [%s:%d]", clientfd, inet_ntoa(peer_addr.sin_addr),
            ntohs(peer_addr.sin_port));
    });

    int i = 0;
    rayrpc::TimerEvent::s_ptr timer_event =
        std::make_shared<rayrpc::TimerEvent>(1000, true, [&i]() { INFOLOG("trigger timer event, count=%d", i++); });


    rayrpc::IOThread io_thread;


    io_thread.getEventLoop()->addEpollEvent(&event);
    io_thread.getEventLoop()->addTimerEvent(timer_event);
    
    // manual start
    io_thread.start();

    // main thread waiting
    io_thread.join();
}

int main() {
    rayrpc::Config::setGlobalConfig("../../rayrpc.xml");

    rayrpc::Logger::initGlobalLogger();

    test_io_thread();

    return 0;
}