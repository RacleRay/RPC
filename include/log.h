#pragma once

#include <memory>
#include <queue>
#include <string>

#include <semaphore.h>

#include "mutex.h"

namespace rayrpc {

// ============================================================================
// marcos
#define DEBUGLOG(str, ...)                                                    \
    if (rayrpc::Logger::getGlobalLogger()->getLogLevel()                      \
        <= rayrpc::LogLevel::Debug) {                                         \
        rayrpc::Logger::getGlobalLogger()->pushLog(                           \
            (rayrpc::LogEvent(rayrpc::LogLevel::Debug)).toString() + "[" \
            + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"  \
            + rayrpc::formatString(str, ##__VA_ARGS__) + "\n");               \
        rayrpc::Logger::getGlobalLogger()->log();                             \
    }

#define INFOLOG(str, ...)                                                    \
    if (rayrpc::Logger::getGlobalLogger()->getLogLevel()                     \
        <= rayrpc::LogLevel::Info) {                                         \
        rayrpc::Logger::getGlobalLogger()->pushLog(                          \
            (rayrpc::LogEvent(rayrpc::LogLevel::Info)).toString() + "[" \
            + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" \
            + rayrpc::formatString(str, ##__VA_ARGS__) + "\n");              \
        rayrpc::Logger::getGlobalLogger()->log();                            \
    }

#define ERRLOG(str, ...)                                                    \
    if (rayrpc::Logger::getGlobalLogger()->getLogLevel()                      \
        <= rayrpc::LogLevel::Error) {                                         \
        rayrpc::Logger::getGlobalLogger()->pushLog(                           \
            (rayrpc::LogEvent(rayrpc::LogLevel::Error)).toString() + "[" \
            + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t"  \
            + rayrpc::formatString(str, ##__VA_ARGS__) + "\n");               \
        rayrpc::Logger::getGlobalLogger()->log();                             \
    }

// ============================================================================
// types

enum class LogLevel { Unknown = 0, Debug = 1, Info = 2, Error = 3 };

// ============================================================================
// functions
template<typename... Args>
std::string formatString(const char *str, Args &&...args) {
    int size = snprintf(nullptr, 0, str, args...);

    std::string result;
    if (size > 0) {
        result.resize(size);
        snprintf(&result[0], size + 1, str, args...);
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

class Logger {
  public:
    using s_ptr = std::shared_ptr<Logger>;
    using Mutex = details::Mutex;

    template<typename T>
    using ScopeMutex = details::ScopeMutex<T>;

    explicit Logger(LogLevel level) : m_level(level) {}

  public:
    LogLevel getLogLevel() const { return m_level; }

    void pushLog(const std::string &msg);

    void log();

    static Logger *getGlobalLogger();

    static void initGlobalLogger();

  private:
    LogLevel m_level; // 日志等级

    std::queue<std::string> m_buffer;

    Mutex m_mutex;
};

} // namespace rayrpc
