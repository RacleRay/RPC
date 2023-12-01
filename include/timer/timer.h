#pragma once

#include <map>

#include "mutex.h"
#include "reactor/fdevent.h"
#include "timer/timerevent.h"


namespace rayrpc {

class Timer : public FdEvent {
public:
    using Mutex = details::Mutex;
    template <class T>
    using ScopedMutex = details::ScopeMutex<T>;

    Timer();
    ~Timer() = default;

    void addTimerEvent(const TimerEvent::s_ptr& timer_event);
    void deleteTimerEvent(TimerEvent::s_ptr imer_event);
    void onTimer();   // TimerEvnet 事件，对应callback

private:
    void resetAlertTime();

private:
    std::multimap<int64_t, TimerEvent::s_ptr> m_pending_events;
    Mutex m_mutex;
};

}  // namespace rayrpc