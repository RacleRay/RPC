#pragma once

#include <vector>

#include "log.h"
#include "iothread/iothread.h"


namespace rayrpc {

class IOThreadGroup {
public:
    explicit IOThreadGroup(int size);
    ~IOThreadGroup() = default;

    void start();
    void join();

    IOThread* getIOThread();

private:
    int m_size{0};
    int m_thread_idx{0};

    std::vector<IOThread*> m_iothread_group;

};  // class IOThreadGroup

}  // namespace rayrpc

