#include "iothread/iothreadgroup.h"


namespace rayrpc {
    
IOThreadGroup::IOThreadGroup(int size) : m_size(size) {
    m_iothread_group.resize(size);
    for (size_t i = 0; i < size; ++i) {
        m_iothread_group[i] = new IOThread();
    }
}


void IOThreadGroup::start() {
    for (auto* iothread: m_iothread_group) {
        iothread->start();
    }        
}

void IOThreadGroup::join() {
    for (auto* iothread: m_iothread_group) {
        iothread->join();
    }
}

// round robin
IOThread* IOThreadGroup::getIOThread() {
    if (m_thread_idx == m_iothread_group.size() || m_thread_idx == -1) {
        m_thread_idx = 0;
    }
    return m_iothread_group[m_thread_idx++];
}

}  // namespace rayrpc