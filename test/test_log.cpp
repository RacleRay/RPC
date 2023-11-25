#include <pthread.h>

#include "config.h"
#include "log.h"

void *fun(void *args) {
    int i = 20;
    while (i--) {
        DEBUGLOG("debug this is thread %s", "fun");
        INFOLOG("info this is thread %s", "fun");
    }
    return nullptr;
}

int main() {
    rayrpc::Config::setGlobalConfig("../../rayrpc.xml");
    rayrpc::Logger::initGlobalLogger();

    pthread_t thread;
    pthread_create(&thread, nullptr, &fun, nullptr);

    int i = 20;
    while (i--) {
        DEBUGLOG("test debug log %s", "11");
        INFOLOG("test info log %s", "11");
    }

    pthread_join(thread, nullptr);
    return 0;
}
