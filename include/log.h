#pragma once

#include <memory>
#include <queue>
#include <semaphore.h>
#include <string>

#include "mutex.h"
#include "timer/timerevent.h"

namespace rayrpc {

// ============================================================================
// marcos
#define DEBUGLOG(str, ...)                                                    \
    if (rayrpc::Logger::getGlobalLogger()->getLogLevel()                      \
        <= rayrpc::LogLevel::Debug) {                                         \
        rayrpc::Logger::getGlobalLogger()->pushLog(                           \
            (rayrpc::LogEvent(rayrpc::LogLevel::Debug)).toString() + "["      \
            + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"  \
            + rayrpc::formatString(str, ##__VA_ARGS__) + "\n");               \
    }

#define INFOLOG(str, ...)                                                    \
    if (rayrpc::Logger::getGlobalLogger()->getLogLevel()                     \
        <= rayrpc::LogLevel::Info) {                                         \
        rayrpc::Logger::getGlobalLogger()->pushLog(                          \
            (rayrpc::LogEvent(rayrpc::LogLevel::Info)).toString() + "["      \
            + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" \
            + rayrpc::formatString(str, ##__VA_ARGS__) + "\n");              \
    }

#define ERRLOG(str, ...)                                                      \
    if (rayrpc::Logger::getGlobalLogger()->getLogLevel()                      \
        <= rayrpc::LogLevel::Error) {                                         \
        rayrpc::Logger::getGlobalLogger()->pushLog(                           \
            (rayrpc::LogEvent(rayrpc::LogLevel::Error)).toString() + "["      \
            + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"  \
            + rayrpc::formatString(str, ##__VA_ARGS__) + "\n");               \
    }


#define APP_DEBUGLOG(str, ...)                                                  \
    if (rayrpc::Logger::getGlobalLogger()->getLogLevel() <= rayrpc::LogLevel::Debug) {  \
        rayrpc::Logger::getGlobalLogger()->pushAppLog(                          \
            rayrpc::LogEvent(rayrpc::LogLevel::Debug).toString() + "["          \
            + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"    \
            + rayrpc::formatString(str, ##__VA_ARGS__) + "\n");                 \
    }

#define APP_INFOLOG(str, ...)                                                   \
    if (rayrpc::Logger::getGlobalLogger()->getLogLevel() <= rayrpc::LogLevel::Info) {  \
        rayrpc::Logger::getGlobalLogger()->pushAppLog(                          \
            rayrpc::LogEvent(rayrpc::LogLevel::Info).toString() + "["           \
            + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"    \
            + rayrpc::formatString(str, ##__VA_ARGS__) + "\n");                 \
    }

#define APP_ERRLOG(str, ...)                                                    \
    if (rayrpc::Logger::getGlobalLogger()->getLogLevel() <= rayrpc::LogLevel::Error) {  \
        rayrpc::Logger::getGlobalLogger()->pushAppLog(                          \
            rayrpc::LogEvent(rayrpc::LogLevel::Error).toString() + "["          \
            + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"    \
            + rayrpc::formatString(str, ##__VA_ARGS__) + "\n");                 \
    }


// ============================================================================
// types

enum class LogLevel { Unknown = 0, Debug = 1, Info = 2, Error = 3 };
enum class LogType { Console = 0, File = 1 };

// ============================================================================
// functions
template<typename... Args>
std::string formatString(const char *str, Args &&...args) {
    if (sizeof...(args) == 0) {
        return std::string(str);
    }

    int size = snprintf(nullptr, 0, str, args...);

    std::string result;
    if (size > 0) {
        result.resize(size);
        snprintf(result.data(), size + 1, str, args...);
    }

    return result;
}

// ============================================================================
// Classes

class LogEvent {
  public:
    explicit LogEvent(LogLevel level) : m_level(level) {}

    std::string getFileName() const { return m_file_name; }

    LogLevel getLogLevel() const { return m_level; }

    std::string toString();

  private:
    std::string m_file_name; // 文件名
    int32_t m_file_line;     // 行号
    int32_t m_pid;           // 进程号
    int32_t m_thread_id;     // 线程号

    LogLevel m_level; // 日志级别
};


class AsyncLogger {
public:
    using s_ptr = std::shared_ptr<AsyncLogger>;
    using Mutex = details::Mutex;
    using ScopeMutex = details::ScopeMutex<Mutex>;

    // filepath/filename
    AsyncLogger(std::string filename, std::string filepath, int max_size);

    void stop();

    void flush();

    void pushLogBuffer(const std::vector<std::string>& buffer);

public:
    static void* fsyncLoop(void* );

public:
    pthread_t m_thread;

private:
    std::queue<std::vector<std::string>> m_buffer;

    sem_t m_semaphore;

    pthread_cond_t m_condtion;  // 条件变量
    Mutex m_mutex;

    int m_num {0};   // 日志文件序号
    std::string m_file_name;    // 日志输出文件名字
    std::string m_file_path;    // 日志输出路径
    std::string m_date;         // 当前打印日志的文件日期
    int m_max_file_size {0};    // 日志单个文件最大大小, 单位为字节

    FILE* m_file_handler {nullptr};   // 当前打开的日志文件句柄

    bool m_reopen_flag {false};
    bool m_stop_flag {false};
};


class Logger {
  public:
    using s_ptr = std::shared_ptr<Logger>;
    using Mutex = details::Mutex;

    template<typename T>
    using ScopeMutex = details::ScopeMutex<T>;

    explicit Logger(LogLevel level, LogType type = LogType::Console);

  public:
    LogLevel getLogLevel() const { return m_level; }

    void pushLog(const std::string &msg);

    void pushAppLog(const std::string &msg);

    void init();  // bug fix

    void log();

    // 异步将日志落盘
    void syncLoop();

    void flush();

    AsyncLogger::s_ptr getAsyncLogger() { return m_async_logger; }

    AsyncLogger::s_ptr getAppAsyncLogger() { return m_async_app_logger; }

  public:
    static Logger *getGlobalLogger();

    static void initGlobalLogger(LogType type = LogType::Console);

  private:
    LogLevel m_level; // 日志等级
    LogType m_type;   // 日志输出类型

    // std::queue<std::string> m_buffer;
    std::vector<std::string> m_buffer;
    std::vector<std::string> m_app_buffer;

    Mutex m_mutex;
    Mutex m_app_mutex;

    AsyncLogger::s_ptr m_async_logger;
    AsyncLogger::s_ptr m_async_app_logger;

    TimerEvent::s_ptr m_fsync_timer_event;
};


} // namespace rayrpc
