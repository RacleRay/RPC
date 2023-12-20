#include <arpa/inet.h>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "iothread/iothreadgroup.h"
#include "log.h"
#include "reactor/eventloop.h"
#include "reactor/fdevent.h"
#include "timer/timerevent.h"


void test_io_thread() {
    // int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    // if (listenfd == -1) {
    //     ERRLOG("listenfd = -1");
    //     std::abort();
    // }

    // sockaddr_in addr;
    // memset(&addr, 0, sizeof(addr));

    // addr.sin_port = htons(12310);
    // addr.sin_family = AF_INET;
    // inet_aton("127.0.0.1", &addr.sin_addr);

    // int rt = bind(listenfd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
    // if (rt != 0) {
    //     ERRLOG("bind error");
    //     std::abort();
    // }

    // rt = listen(listenfd, 100);
    // if (rt != 0) {
    //     ERRLOG("listen error");
    //     std::abort();
    // }

    // rayrpc::FdEvent event(listenfd);
    // event.listen(rayrpc::FdEvent::TriggerEvent::IN_EVENT, [listenfd]() {
    //     sockaddr_in peer_addr;
    //     socklen_t addr_len = sizeof(peer_addr);
    //     memset(&peer_addr, 0, sizeof(peer_addr));
    //     int clientfd = accept(listenfd, reinterpret_cast<sockaddr *>(&peer_addr), &addr_len);
    //     DEBUGLOG(
    //         "success get client fd[%d], peer addr: [%s:%d]", clientfd, inet_ntoa(peer_addr.sin_addr),
    //         ntohs(peer_addr.sin_port));
    // });

    int i = 0;
    rayrpc::TimerEvent::s_ptr timer_event =
        std::make_shared<rayrpc::TimerEvent>(1000, true, [&i]() { INFOLOG("Timer::onTimer: trigger timer event, count=%d", i++); });

    // build io thread group
    rayrpc::IOThreadGroup io_thread_group(2);
    rayrpc::IOThread* io_thread_1 = io_thread_group.getIOThread();
    io_thread_1->getEventLoop()->addTimerEvent(timer_event);
    // io_thread_1->getEventLoop()->addEpollEvent(&event);

    rayrpc::IOThread* io_thread_2 = io_thread_group.getIOThread();
    io_thread_2->getEventLoop()->addTimerEvent(timer_event);

    io_thread_group.start();
    io_thread_group.join();
}

int main() {
    rayrpc::Config::setGlobalConfig("../../rayrpc.xml");

    rayrpc::Logger::initGlobalLogger();

    test_io_thread();

    return 0;
}