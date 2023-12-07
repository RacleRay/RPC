#include "fdeventgroup.h"

namespace {

rayrpc::FdEventGroup *g_fd_event_group = nullptr;

constexpr size_t fdevent_group_default_size = 128;

} // namespace


namespace rayrpc {

FdEventGroup::FdEventGroup(int size) : m_size(size) {
    for (int i = 0; i < m_size; i++) { 
        m_fd_group.push_back(new FdEvent(i)); 
    }
}

FdEventGroup::~FdEventGroup() {
    for (int i = 0; i < m_size; ++i) {
        if (m_fd_group[i] != nullptr) {
            delete m_fd_group[i];
            m_fd_group[i] = nullptr;
        }
    }
}

FdEvent *FdEventGroup::getFdEvent(int fd) {
    ScopeMutex lock(m_mutex);
    if ((size_t)fd < m_fd_group.size()) { 
        return m_fd_group[fd];
    }

    // adjust the fd group size
    int new_size = int(fd * 1.5);
    for (int i = (int)m_fd_group.size(); i < new_size; ++i) {
        m_fd_group.push_back(new FdEvent(i));
    }

    return m_fd_group[fd];
}

// static
FdEventGroup *FdEventGroup::GetFdEventGroup() {
    if (g_fd_event_group != nullptr) { 
        return g_fd_event_group;
    }
    g_fd_event_group = new FdEventGroup(fdevent_group_default_size);
    return g_fd_event_group;
}


} // namespace rayrpc