#include <cassert>

#include "log.h"
#include "iothread/iothread.h"
#include "utils.h"


namespace rayrpc {

IOThread::IOThread() {
    int ret = sem_init(&m_init_semaphore, 0, 0);
    assert(ret == 0);
    ret = sem_init(&m_start_semaphore, 0, 0);
    assert(ret == 0);

    pthread_create(&m_thread, nullptr, main_s, this);
    sem_wait(&m_init_semaphore); // wait main_s to be complete

    DEBUGLOG("IOThread::IOThread created thread [%d].", m_thread_id);
}

IOThread::~IOThread() {
    m_eventloop->stop();

    sem_destroy(&m_init_semaphore);
    sem_destroy(&m_start_semaphore);

    // main thread wait child to be recycled.
    pthread_join(m_thread, nullptr);

    if (m_eventloop) {
        delete m_eventloop;
        m_eventloop = nullptr;
    }
}

// child thread works
void *IOThread::main_s(void *arg) {
    auto* thread = static_cast<IOThread*>(arg);

    thread->m_eventloop = new EventLoop();
    thread->m_thread_id = details::get_thread_id();

    sem_post(&thread->m_init_semaphore);

    // wait start instruction from main thread.
    DEBUGLOG("IOThread::main_s, thread [%d] created, wait start semaphore from main thread.", thread->m_thread_id);

    sem_wait(&thread->m_start_semaphore);
    
    DEBUGLOG("IOThread::main_s, thread [%d] start loop.", thread->m_thread_id);
    thread->m_eventloop->loop();
    DEBUGLOG("IOThread::main_s, thread [%d] exit loop.", thread->m_thread_id);

    return nullptr;
}

EventLoop *IOThread::getEventLoop() const noexcept {
    return m_eventloop;
}

void IOThread::start() {
    DEBUGLOG("IOThread::start, semapthore invoke thread [%d].", m_thread_id);
    sem_post(&m_start_semaphore);
}

// main thread wait child end.
void IOThread::join() const {
    pthread_join(m_thread, nullptr);
}

} // namespace rayrpc