#include <cstring>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>

#include "log.h"
#include "reactor/eventloop.h"
#include "utils.h"

namespace {

thread_local rayrpc::EventLoop *t_current_loop = nullptr;

int g_epoll_max_timeout = 10000; // ms
int g_epoll_max_events = 10;


inline void add_to_epoll(std::set<int> &listen_fds, rayrpc::FdEvent *event, int epoll_fd) {
    auto it = listen_fds.find(event->getFd());
    int op = EPOLL_CTL_ADD;
    if (it != listen_fds.end()) { op = EPOLL_CTL_MOD; }

    epoll_event tmp = event->getEpollEvent();
    INFOLOG("EventLoop::addEpollEvent: add epoll_event.events = %d", (int)tmp.events);

    int ret = epoll_ctl(epoll_fd, op, event->getFd(), &tmp);
    if (ret < 0) { ERRLOG("EventLoop::addEpollEvent: failed epoll_ctl when add fd, errno=%d, error=%s", errno, strerror(errno)); }

    listen_fds.insert(event->getFd());
    DEBUGLOG("EventLoop::addEpollEvent: add event success, fd[%d]", event->getFd());
}


inline void remove_from_epoll(std::set<int> &listen_fds, rayrpc::FdEvent *event, int epoll_fd) {
    auto it = listen_fds.find(event->getFd());
    if (it == listen_fds.end()) { return; }

    int op = EPOLL_CTL_DEL;
    epoll_event tmp = event->getEpollEvent();
    int ret = epoll_ctl(epoll_fd, op, event->getFd(), nullptr);
    if (ret < 0) { ERRLOG("EventLoop::deleteEpollEvent: failed epoll_ctl when add fd, errno=%d, error=%s", errno, strerror(errno)); }

    listen_fds.erase(event->getFd());  // remove from epoll listen fdss
    DEBUGLOG("EventLoop::deleteEpollEvent: delete event success, fd[%d]", event->getFd());
}

} // namespace

namespace rayrpc {

EventLoop::EventLoop() : m_thread_id(details::get_thread_id()) {
    if (t_current_loop != nullptr) {
        ERRLOG("EventLoop::EventLoop: failed to create event loop, this thread has created event loop");
        std::abort();
    }

    // epoll of current eventloop
    m_epoll_fd = epoll_create(10);
    if (m_epoll_fd == -1) {
        ERRLOG("EventLoop::EventLoop: failed to create event loop, epoll_create error, error info[%d]", errno);
        std::abort();
    }

    // registe the thread wakeup fd for processing the pending tasks.
    initWakeUpFdEevent();
    initTimer();

    // thread local eventloop
    t_current_loop = this;
    INFOLOG("EventLoop::EventLoop: successfully create event loop in thread [%d]", m_thread_id);
}

EventLoop::~EventLoop() {
    close(m_epoll_fd);
    if (m_wakeup_fd_event) {
        delete m_wakeup_fd_event;
        m_wakeup_fd_event = nullptr;
    }
    if (m_timer) {
        delete m_timer;
        m_timer = nullptr;
    }
}


void EventLoop::initWakeUpFdEevent() {
    m_wakeup_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_wakeup_fd < 0) {
        ERRLOG("EventLoop::initWakeUpFdEevent: failed to create event loop, eventfd create error, error info [%d]", errno);
        std::abort();
    }
    INFOLOG("EventLoop::initWakeUpFdEevent: wakeup fd = %d", m_wakeup_fd);

    // init wakeup fd and its read event callback
    m_wakeup_fd_event = new WakeUpFdEvent(m_wakeup_fd);
    m_wakeup_fd_event->listen(FdEvent::TriggerEvent::IN_EVENT, [this]() {
        int64_t val;
        while (read(m_wakeup_fd, &val, sizeof(val)) > 0 && errno != EAGAIN) {
            // wait for read complele.
        }
        DEBUGLOG("EventLoop::initWakeUpFdEevent: read full bytes from wakeup fd [%d]", m_wakeup_fd);
    });

    addEpollEvent(m_wakeup_fd_event);
}


void EventLoop::wakeup() {
    INFOLOG("EventLoop::wakeup: wakeup event loop in thread [%d]", m_thread_id);
    m_wakeup_fd_event->wakeup();
}


void EventLoop::stop() {
    m_stop_flag = true;
}


void EventLoop::addEpollEvent(FdEvent *event) {
    if (isInLoopThread()) {
        add_to_epoll(m_listen_fds, event, m_epoll_fd);
    } else {
        auto cb = [this, event]() {
            add_to_epoll(m_listen_fds, event, m_epoll_fd);
        };
        addTask(std::move(cb), true);
    }
}


void EventLoop::deleteEpollEvent(FdEvent *event) {
    if (isInLoopThread()) {
        remove_from_epoll(m_listen_fds, event, m_epoll_fd);
    } else {
        auto cb = [this, event]() {
            remove_from_epoll(m_listen_fds, event, m_epoll_fd);
        };
        addTask(std::move(cb), true);
    }
}


void EventLoop::addTask(std::function<void()> &&cb, bool to_wake_up) {
    {
        ScopeMutex<Mutex> lock(m_mutex);
        m_pending_tasks.push(std::move(cb));
    }

    if (to_wake_up) { wakeup(); }
}


bool EventLoop::isInLoopThread() const {
    return details::get_thread_id() == m_thread_id;
}


void EventLoop::initTimer() {
    m_timer = new Timer();
    addEpollEvent(m_timer);  // add timer fd to epoll
}


void EventLoop::addTimerEvent(TimerEvent::s_ptr timer_event) {
    m_timer->addTimerEvent(timer_event);
}


void EventLoop::loop() {
    while (!m_stop_flag) {
        std::queue<std::function<void()>> tmp_tasks;
        {
            ScopeMutex<Mutex> lock(m_mutex);
            m_pending_tasks.swap(tmp_tasks);
        }

        while (!tmp_tasks.empty()) {
            std::function<void()> task = tmp_tasks.front();
            tmp_tasks.pop();
            if (task) { task(); }   // processing pending tasks
        }

        int timeout = g_epoll_max_timeout;
        epoll_event result_event[g_epoll_max_events];

        int ret = epoll_wait(m_epoll_fd, result_event, g_epoll_max_events, timeout);
        DEBUGLOG("EventLoop::loop: return from epoll_wait, ret = [%d]", ret);

        if (ret < 0) {
            ERRLOG("EventLoop::loop: epoll_wait error, errno [%d], error info [%s]", errno, strerror(errno));
            continue;
        }

        for (int i = 0; i < ret; i++) {
            epoll_event trigger_event = result_event[i];
            auto *fdevent = static_cast<FdEvent *>(trigger_event.data.ptr);
            if (!fdevent) {
                ERRLOG("EventLoop::loop: fdevent is nullptr");
                continue;
            }

            // add corresponding tasks to current loops pending task queue.
            if (trigger_event.events & EPOLLIN) {
                DEBUGLOG("EventLoop::loop: fd [%d] trigger EPOLLIN event", fdevent->getFd())
                addTask(fdevent->handler(FdEvent::TriggerEvent::IN_EVENT));
            }
            if (trigger_event.events & EPOLLOUT) {
                DEBUGLOG("EventLoop::loop: fd [%d] trigger EPOLLOUT event", fdevent->getFd())
                addTask(fdevent->handler(FdEvent::TriggerEvent::OUT_EVENT));
            }
        }
    }
}

// static 
EventLoop* EventLoop::GetCurrentEventLoop() {
    if (t_current_loop) {
        return t_current_loop;
    }
    t_current_loop = new EventLoop();
    return t_current_loop;
}

} // namespace rayrpc