#pragma once

#include <exception>
#include <string>

namespace rayrpc {
    
class RayrpcException : public std::exception {
public:
    RayrpcException() = default;

    ~RayrpcException() override = default;

    // 当 EventLoop 捕获到 RayrpcException 及其子类对象的异常时，会执行该函数
    virtual void handle() = 0;

    int getErrorCode() const { return m_error_code; }

    std::string getErrorInfo() const { return m_error_info; }

protected:
    int m_error_code{0};
    std::string m_error_info;
};

}  // namespace rayrpc