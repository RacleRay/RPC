#include <cstdio>
#include <map>
#include <sstream>

#include <sys/time.h>

#include "config.h"
#include "log.h"
#include "utils.h"

namespace {
rayrpc::Logger *g_logger = nullptr;

std::map<rayrpc::LogLevel, std::string> loglevel_to_string = {
    {rayrpc::LogLevel::Debug, "DEBUG"},
    {rayrpc::LogLevel::Info, "INFO"},
    {rayrpc::LogLevel::Error, "ERROR"}
};


std::map<std::string, rayrpc::LogLevel> string_to_loglevel = {
    {"Debug", rayrpc::LogLevel::Debug},
    {"Info", rayrpc::LogLevel::Info},
    {"Error", rayrpc::LogLevel::Error}
};


}  // namespace 

namespace rayrpc {

// ============================================================================
// Logger
Logger *Logger::getGlobalLogger() {
    return g_logger;
}

void Logger::initGlobalLogger() {
    LogLevel global_log_level = string_to_loglevel[Config::getGlobalConfig()->m_log_level];
    printf("Init log level [%s]\n", loglevel_to_string[global_log_level].c_str());
    g_logger = new Logger(global_log_level);
}

void Logger::pushLog(const std::string &msg) {
    ScopeMutex<Mutex> lock(m_mutex);
    m_buffer.push(msg);
}

void Logger::log() {
    // swap to buffer, to unlock quickly
    std::queue<std::string> tmp;
    {
        ScopeMutex<Mutex> lock(m_mutex);
        m_buffer.swap(tmp);
    }

    std::string msg;
    while (!tmp.empty()) {
        msg = tmp.front();
        tmp.pop();
        printf("%s", msg.c_str());
    }
}

// =========================================================================
// LogEvent
// Format :  [level] [time] [pid:tid] msg
std::string LogEvent::toString() {
    // get time string
    struct timeval now_time;
    gettimeofday(&now_time, nullptr);

    struct tm now_time_t;
    localtime_r(&(now_time.tv_sec), &now_time_t);

    char buf[128];
    (void)strftime(&buf[0], 128, "%y-%m-%d %H:%M:%S", &now_time_t);
    std::string time_str(buf);
    
    auto ms = now_time.tv_usec / 1000;
    time_str = time_str + "." + std::to_string(ms);

    // get pid, tid
    m_pid = details::get_pid();
    m_thread_id = details::get_thread_id();

    // output
    std::stringstream ss;

    ss << "[" << loglevel_to_string[m_level] << "]\t"
        << "[" << time_str << "]\t"
        << "[" << m_pid << ":" << m_thread_id << "]\t";

    return ss.str();
}

} // namespace rayrpc