#pragma once

#include <pthread.h>
#include <semaphore.h>

#include "reactor/eventloop.h"


namespace rayrpc {
    
class IOThread {
public:
    IOThread();
    ~IOThread();

    EventLoop* getEventLoop() const noexcept ;
    
    void start();
    void join() const;

public:
    static void* main_s(void * arg);

private:
    pid_t m_thread_id {0};
    pthread_t m_thread {0};

    EventLoop* m_eventloop {nullptr};  // every IOThread has one EventLoop

    sem_t m_init_semaphore;
    
    sem_t m_start_semaphore;  // 用于在启动前手动插入任务，然后主动启动 loop 循环

};  // class IOThread

}  // namespace rayrpc