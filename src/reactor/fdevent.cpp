#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include "reactor/fdevent.h"
#include "log.h"


namespace rayrpc {

// ============================================================================
// FdEvent 

std::function<void()> FdEvent::handler(TriggerEvent event_type) {
    if (event_type == TriggerEvent::IN_EVENT) {
        return m_read_callback;
    }
    return m_write_callback;
}

void FdEvent::listen(TriggerEvent event_type, const std::function<void()>& callback) {
    if (event_type == TriggerEvent::IN_EVENT) {
        m_listen_events.events |= EPOLLIN;
        m_read_callback = callback;
    } else {
        m_listen_events.events |= EPOLLOUT;
        m_write_callback = callback;
    }
    m_listen_events.data.ptr = this;
}


void FdEvent::setNonBlock() {
    int flag = fcntl(m_fd, F_GETFL, 0);
    if (flag & O_NONBLOCK) {
        return;
    }
    fcntl(m_fd, F_SETFL, flag | O_NONBLOCK);
}


void FdEvent::cancel(TriggerEvent event_type) {
    if (event_type == TriggerEvent::IN_EVENT) {
        m_listen_events.events &= ~EPOLLIN;
    } else {
        m_listen_events.events &= ~EPOLLOUT;
    }
}

// ============================================================================
// WakeUpFdEvent
void WakeUpFdEvent::wakeup() {
    int64_t val = 1;
    ssize_t ret = write(m_fd, &val, sizeof(val));
    if (ret != 8) {
        ERRLOG("write to wakeup fd less than 8 bytes, fd[%d]", m_fd);
    }
    DEBUGLOG("WakeUpFdEvent::wakeup: success read 8 bytes");
}

}  // namespace rayrpc