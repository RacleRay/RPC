#pragma once

#include <vector>

#include "log.h"
#include "iothread/iothread.h"


namespace rayrpc {

class IOThreadGroup {
public:
    explicit IOThreadGroup(size_t size);
    ~IOThreadGroup() = default;

    void start();
    void join();

    IOThread* getIOThread();

private:
    size_t m_size{0};
    size_t m_thread_idx{0};

    std::vector<IOThread*> m_iothread_group;

};  // class IOThreadGroup

}  // namespace rayrpc

