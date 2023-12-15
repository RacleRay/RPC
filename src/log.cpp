#include <cassert>
#include <cstdio>
#include <map>
#include <sstream>
#include <sys/time.h>

#include "config.h"
#include "log.h"
#include "reactor/eventloop.h"
#include "runinfo.h"
#include "utils.h"

namespace {
rayrpc::Logger *g_logger = nullptr;

std::map<rayrpc::LogLevel, std::string> loglevel_to_string = {
    {rayrpc::LogLevel::Unknown, "Unknown"},
    {rayrpc::LogLevel::Debug, "DEBUG"},
    {rayrpc::LogLevel::Info, "INFO"},
    {rayrpc::LogLevel::Error, "ERROR"}
};


std::map<std::string, rayrpc::LogLevel> string_to_loglevel = {
    {"Unknown", rayrpc::LogLevel::Unknown},
    {"Debug", rayrpc::LogLevel::Debug},
    {"Info", rayrpc::LogLevel::Info},
    {"Error", rayrpc::LogLevel::Error}
};


}  // namespace 

namespace rayrpc {

// ============================================================================
// Logger
Logger::Logger(LogLevel level, LogType type): m_level(level), m_type(type) {{
    if (m_type == LogType::Console) {
        return;
    }

    m_async_logger = std::make_shared<AsyncLogger> (
        Config::getGlobalConfig()->m_log_file_name + "_rpc",
        Config::getGlobalConfig()->m_log_file_path,
        Config::getGlobalConfig()->m_log_max_file_size
    );
    m_async_app_logger = std::make_shared<AsyncLogger> (
        Config::getGlobalConfig()->m_log_file_name + "_app",
        Config::getGlobalConfig()->m_log_file_path,
        Config::getGlobalConfig()->m_log_max_file_size
    );
}}

Logger *Logger::getGlobalLogger() {
    return g_logger;
}

void Logger::init() {
    if (m_type == LogType::Console) {
        return;
    }

    m_fsync_timer_event = std::make_shared<TimerEvent> (
        Config::getGlobalConfig()->m_log_sync_inteval, 
        true,
        [this] () { syncLoop(); }
    );
    EventLoop::GetCurrentEventLoop()->addTimerEvent(m_fsync_timer_event);
}

void Logger::initGlobalLogger(LogType type /*= LogType::Console*/) {
    LogLevel global_log_level = string_to_loglevel[Config::getGlobalConfig()->m_log_level];
    printf("Init log level [%s]\n", loglevel_to_string[global_log_level].c_str());
    g_logger = new Logger(global_log_level, type);
    g_logger->init();  // init timer event
}

void Logger::pushLog(const std::string &msg) {
    if (m_type == LogType::Console) {
        printf("%s", (msg + "\n").c_str());
        return;
    }

    ScopeMutex<Mutex> lock(m_mutex);
    m_buffer.push_back(msg);
}

void Logger::pushAppLog(const std::string &msg) {
    ScopeMutex<Mutex> lock(m_app_mutex);
    m_app_buffer.push_back(msg);
}


void Logger::log() {
    // swap to buffer, to unlock quickly
    std::vector<std::string> tmp;
    {
        ScopeMutex<Mutex> lock(m_mutex);
        m_buffer.swap(tmp);
    }

    std::string msg;
    while (!tmp.empty()) {
        msg = tmp.front();
        tmp.erase(tmp.begin());
        printf("%s", msg.c_str());
    }
}

// 同步 m_buffer 到 async_logger 的队尾
void Logger::syncLoop() {
    std::vector<std::string> tmp;
    
    m_mutex.lock();
    tmp.swap(m_buffer);
    m_mutex.unlock();

    if (!tmp.empty()) {
        m_async_logger->pushLogBuffer(tmp);
    }
    tmp.clear();

    m_app_mutex.lock();
    tmp.swap(m_app_buffer);
    m_app_mutex.unlock();

    if (!tmp.empty()) {
        m_async_app_logger->pushLogBuffer(tmp);
    }
}


// =========================================================================
// AsyncLogger
AsyncLogger::AsyncLogger(std::string filename, std::string filepath, int max_size)
    : m_file_name(std::move(filename)), m_file_path(std::move(filepath)), m_max_file_size(max_size) 
{
    sem_init(&m_semaphore, 0, 0);
    assert(pthread_create(&m_thread, nullptr, fsyncLoop, this) == 0);
    sem_wait(&m_semaphore);  // wait pthread init complete
}

void AsyncLogger::stop() {
    m_stop_flag = true;
}

void AsyncLogger::flush() {
    if (m_file_handler) {
        (void)fflush(m_file_handler);
    }
}

void AsyncLogger::pushLogBuffer(const std::vector<std::string>& buffer) {
    {
        ScopeMutex lock(m_mutex);
        m_buffer.push(buffer);
        pthread_cond_signal(&m_condtion);
    }
}

void* AsyncLogger::fsyncLoop(void* arg) {
    auto* logger = reinterpret_cast<AsyncLogger*>(arg);
    
    assert(pthread_cond_init(&logger->m_condtion, nullptr) == 0);
    sem_post(&logger->m_semaphore);  // pthread init complete

    while (true) {
        std::vector<std::string> tmp;
        {
            ScopeMutex lock(logger->m_mutex);
            while (logger->m_buffer.empty()) {
                pthread_cond_wait(&logger->m_condtion, logger->m_mutex.getMutex());
            }
            tmp.swap(logger->m_buffer.front());
            logger->m_buffer.pop();
        }

        timeval now;
        gettimeofday(&now, nullptr);

        struct tm now_tm;
        localtime_r(&(now.tv_sec), &now_tm);
        
        const char* format = "%Y-%m-%d";
        char date[32];
        (void)strftime(date, 32, format, &now_tm);
        
        // check if need to open a new file
        if (std::string(date) != logger->m_date) {
            logger->m_num = 0;
            logger->m_reopen_flag = true;
            logger->m_date = std::string(date);
        }
        if (logger->m_file_handler == nullptr) {
            logger->m_reopen_flag = true;
        }

        // construct log file name
        std::stringstream ss;
        ss << logger->m_file_path << logger->m_file_name << "_" << std::string(date) 
            << "_log.";
        std::string log_file_name = ss.str() + std::to_string(logger->m_num);

        // open file
        if (logger->m_reopen_flag) {
            if (logger->m_file_handler) {
                (void)fclose(logger->m_file_handler);
            }
            logger->m_file_handler = fopen(log_file_name.c_str(), "a");
            logger->m_reopen_flag = false;
        }

        // exceed limit
        if (ftell(logger->m_file_handler) > logger->m_max_file_size) {
            (void)fclose(logger->m_file_handler);
            log_file_name = ss.str() + std::to_string(logger->m_num++);
            logger->m_file_handler = fopen(log_file_name.c_str(), "a");
            logger->m_reopen_flag = false;
        }

        // write to file
        for (auto& msg : tmp) {
            if (!msg.empty()) {
                (void)fwrite(msg.c_str(), 1, msg.size(), logger->m_file_handler);
            }
        }
        logger->flush();

        if (logger->m_stop_flag) {
            return nullptr;
        }
    }

    return nullptr;
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

    // get run time info
    std::string req_id = RunInfo::GetRunInfo()->m_req_id;
    std::string method_name = RunInfo::GetRunInfo()->m_method_name;
    if (!req_id.empty()) {
        ss << "[" << req_id << "]\t";
    }

    if (!method_name.empty()) {
        ss << "[" << method_name << "]\t";
    }
    return ss.str();
}

} // namespace rayrpc