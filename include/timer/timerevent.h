#pragma once

#include <cstdint>
#include <functional>
#include <memory>


namespace rayrpc {

class TimerEvent {
public:
    using s_ptr = std::shared_ptr<TimerEvent>;

    TimerEvent(int64_t interval, bool is_repeated, std::function<void()> task);

    int64_t getAlertTime() const noexcept {
        return m_alert_time;
    }

    void setCanceled(bool is_cancel)  noexcept {
        m_is_canceled = is_cancel;
    }

    bool isCanceled() const noexcept {
        return m_is_canceled;
    }

    bool isRepeated() const noexcept {
        return m_is_repeated;
    }

    std::function<void()> getTask() const noexcept {
        return m_task;
    }

    void resetAlertTime();

private:
    int64_t m_alert_time;
    int64_t m_interval;   // ms
    bool m_is_repeated{false};  // 重复触发
    bool m_is_canceled{false};  // 是否已取消

    std::function<void()> m_task;
};

}  // namespace rayrpc