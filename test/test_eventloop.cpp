#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "log.h"
#include "reactor/eventloop.h"
#include "reactor/fdevent.h"
#include "timer/timer_event.h"


int main() {
    rayrpc::Config::setGlobalConfig("../../rayrpc.xml");
    rayrpc::Logger::initGlobalLogger();

    auto* event_loop = new rayrpc::EventLoop();

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        ERRLOG("socket error"); 
        return -1;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_port = htons(8888);
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr);

    int ret = bind(listenfd, (sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        ERRLOG("bind error");
        return -2;
    }

    ret = listen(listenfd, 5);
    if (ret < 0) {
        ERRLOG("listen error");
        return -3;
    }

    // setup listenfd event
    rayrpc::FdEvent event(listenfd);
    event.listen(rayrpc::FdEvent::TriggerEvent::IN_EVENT, [listenfd]() {
        sockaddr_in peer_addr;
        socklen_t addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, addr_len);

        int clientfd = accept(listenfd, (sockaddr*)&peer_addr, &addr_len);
        DEBUGLOG("success get client fd[%d], peer addr: [%s:%d]", clientfd, inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
    });
    event_loop->addEpollEvent(&event);

    // setup timer event
    int i = 0;
    rayrpc::TimerEvent::s_ptr timer_event = std::make_shared<rayrpc::TimerEvent>(
        1000, true, [&i]() {
            INFOLOG("trigger timer event, count: [%d]", i++);
        }
    );
    event_loop->addTimerEvent(timer_event);

    // eventloop run
    event_loop->loop();

    return 0;
}