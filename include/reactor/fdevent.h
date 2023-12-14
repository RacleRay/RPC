#pragma once

#include <functional>
#include <sys/epoll.h>
#include <unistd.h>

namespace rayrpc {

class FdEvent {
  public:
    enum class TriggerEvent {
        IN_EVENT = EPOLLIN,
        OUT_EVENT = EPOLLOUT,
        ERR_EVENT = EPOLLERR,
    };

    FdEvent() = default;
    explicit FdEvent(int fd) : m_fd(fd) {};

    ~FdEvent() {
        if (m_fd > 0) {
            close(m_fd);
        }
    };

    [[nodiscard]] int getFd() const { return m_fd; }
    epoll_event getEpollEvent() { return m_listen_events; }

    // invoke callback which is registed in listen member function.
    std::function<void()> handler(TriggerEvent event_type);
    
    // register specific event and its callback
    void listen(TriggerEvent event_type, const std::function<void()>& callback, const std::function<void()>& err_callback = nullptr);

    void setNonBlock();

    void cancel(TriggerEvent event_type);

  protected:
    int m_fd{-1};

    epoll_event m_listen_events{0};

    std::function<void()> m_read_callback{nullptr};
    std::function<void()> m_write_callback{nullptr};
    std::function<void()> m_err_callback{nullptr};
};


// ============================================================================
class WakeUpFdEvent : public FdEvent {
  public:
    explicit WakeUpFdEvent(int fd) : FdEvent(fd) {};

    ~WakeUpFdEvent() = default;

    void wakeup();
};

} // namespace rayrpc