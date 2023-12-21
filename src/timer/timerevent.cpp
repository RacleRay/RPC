#include <timer/timerevent.h>

#include "log.h"
#include "utils.h"

namespace rayrpc {

TimerEvent::TimerEvent(int64_t interval, bool is_repeated, std::function<void()> task)
    : m_interval(interval), m_is_repeated(is_repeated), m_task(std::move(task)) 
{
    resetAlertTime();
}


void TimerEvent::resetAlertTime() {
    m_alert_time = details::get_now_time() + m_interval;
    DEBUGLOG("TimerEvent::resetAlertTime: success create timer event, will excute at [%lld]", m_alert_time);
}

}  // namespace rayrpc