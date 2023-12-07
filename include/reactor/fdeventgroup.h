#pragma once

#include <vector>

#include "fdevent.h"
#include "mutex.h"



namespace rayrpc {

// fd event 管理类，按照 fd 作为 index 组织
class FdEventGroup {
  public:
    using Mutex = details::Mutex;
    using ScopeMutex = details::ScopeMutex<Mutex>;

    explicit FdEventGroup(int size);
    ~FdEventGroup();

    FdEvent* getFdEvent(int fd);

  public:
    static FdEventGroup* GetFdEventGroup();

  private:
    size_t m_size;
    
    std::vector<FdEvent*> m_fd_group;

    Mutex m_mutex;
};

} // namespace rayrpc