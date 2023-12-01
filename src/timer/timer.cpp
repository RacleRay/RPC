#include <cstring>
#include <sys/timerfd.h>

#include "timer/timer.h"
#include "log.h"
#include "utils.h"


namespace rayrpc {

Timer::Timer() {
    m_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

    DEBUGLOG("Timer::Timer fd=%d", m_fd);

    listen(FdEvent::TriggerEvent::IN_EVENT, 
        [this]() {onTimer();});
}


// 更新 timerfd 设置的 timeout 时间
void Timer::resetAlertTime() {
    ScopedMutex<Mutex> lock(m_mutex);
    auto tmp = m_pending_events;
    lock.unlock();

    if (tmp.empty()) {
        return;
    }

    int64_t now = details::get_now_time();
    int64_t interval = 0;
    auto it = tmp.begin();   // 已排序
    if (it->second->getAlertTime() > now) {   // 还没有到TimeOut触发时间
        interval = it->second->getAlertTime() - now;
    } else {  // 已经超时了，reset 到 100 ms
        interval = 100;
    }

    timespec ts;
    memset(&ts, 0, sizeof(ts));
    ts.tv_sec = interval / 1000;
    ts.tv_nsec = (interval % 1000) * 1000000;

    itimerspec value;
    memset(&value, 0, sizeof(value));
    value.it_value = ts;
    int ret = timerfd_settime(m_fd, 0, &value, nullptr);
    if (ret != 0) {
        ERRLOG("timerfd_settime error, errno=%d, error=%s", errno, strerror(errno));
    }
    DEBUGLOG("Timer::resetAlertTime interval=%ld", interval);
}


void Timer::addTimerEvent(const TimerEvent::s_ptr& timer_event) {
    bool is_reset_timerfd = false;   // 是否需要更新 timerfd 的timeout时间

    ScopedMutex<Mutex> lock(m_mutex);
    if (m_pending_events.empty()) {  // 第一个插入任务
        is_reset_timerfd = true;
    } else {
        auto it = m_pending_events.begin();
        if ((*it).second->getAlertTime() > timer_event->getAlertTime()) {  // timeout 更早的事件 
            is_reset_timerfd = true;
        }
    }
    m_pending_events.emplace(timer_event->getAlertTime(), timer_event);
    lock.unlock();
    
    // 更新 timerfd 的timeout时间
    if (is_reset_timerfd) {
        resetAlertTime();
    }
}


void Timer::deleteTimerEvent(TimerEvent::s_ptr timer_event) {
    timer_event->setCanceled(true);

    ScopedMutex<Mutex> lock{m_mutex};
    
    int64_t time_key = timer_event->getAlertTime();
    auto begin = m_pending_events.lower_bound(time_key);
    auto end = m_pending_events.upper_bound(time_key);

    auto it = begin;
    for (it = begin; it != end; ++it) {
        if (it->second == timer_event) {
            break;
        }
    }

    if (it != end) {
        m_pending_events.erase(it);
    }

    lock.unlock();

    DEBUGLOG("Timer::deleteTimerEvent time_key=%ld", time_key);
}

void Timer::onTimer() {
    DEBUGLOG("Timer::onTimer");

    char buf[8];
    while (true) {
        // read complete
        if ((read(m_fd, buf, 8) == -1) && (errno == EAGAIN)) {
            break;
        }
    }

    // add tasks
    int64_t now = details::get_now_time();

    std::vector<TimerEvent::s_ptr> tout_events;
    std::vector< std::pair<int64_t, std::function<void()>> > tasks;

    ScopedMutex<Mutex> lock(m_mutex);

    auto it = m_pending_events.begin();
    for (it = m_pending_events.begin(); it != m_pending_events.end(); ++it) {
        auto map_item = *it;
        if (map_item.first <= now) {   // timeout
            if (!map_item.second->isCanceled()) {
                tout_events.push_back(map_item.second);
                tasks.emplace_back(map_item.first, map_item.second->getTask());
            }
        } else {
            break;
        }
    }
    // remove from pending timer events.
    m_pending_events.erase(m_pending_events.begin(), it);
    lock.unlock();

    // readd the repeated timer evnets
    for (auto i = tout_events.begin(); i != tout_events.end(); ++i) {
        auto timer_event = *i;
        if (timer_event->isRepeated()) {
            timer_event->resetAlertTime();
            addTimerEvent(timer_event);
        }
    }

    resetAlertTime();

    // do tasks
    for (const auto& item: tasks) {
        if (item.second) {
            item.second();
        }
    }
}

}  // namespace rayrpc