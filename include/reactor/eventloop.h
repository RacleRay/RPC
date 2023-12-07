#pragma once

#include <functional>
#include <queue>
#include <set>

#include <pthread.h>

#include "fdevent.h"
#include "mutex.h"
#include "timer/timer.h"
#include "timer/timerevent.h"

namespace rayrpc {

class EventLoop {
  public:
    template<typename T>
    using ScopeMutex = details::ScopeMutex<T>;
    using Mutex = details::Mutex;

    EventLoop();

    ~EventLoop();

    void loop();

    void wakeup();

    void stop();

    void addEpollEvent(FdEvent *event);

    void deleteEpollEvent(FdEvent *event);

    bool isInLoopThread() const;

    void addTask(std::function<void()>&& cb, bool to_wake_up = false);

    void addTimerEvent(TimerEvent::s_ptr event);

  public:
    static EventLoop* GetCurrentEventLoop();

  private:
    void dealWakeup();

    void initWakeUpFdEevent();

    void initTimer();

  private:
    pid_t m_thread_id{0};   // 用于

    int m_epoll_fd{0};

    int m_wakeup_fd{0};

    WakeUpFdEvent *m_wakeup_fd_event{nullptr};

    bool m_stop_flag{false};

    std::set<int> m_listen_fds;   // listening fds in current loop

    std::queue<std::function<void()>> m_pending_tasks;

    Mutex m_mutex;

    Timer* m_timer{nullptr};
};

} // namespace rayrpc
