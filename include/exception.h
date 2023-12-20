#pragma once

#include <exception>
#include <string>

namespace rayrpc {
    
class RayrpcException : public std::exception {
public:
    RayrpcException(int err_code, std::string err_info)
        : m_err_code(err_code), m_err_info(std::move(err_info)) {};

    ~RayrpcException() override = default;

    [[nodiscard]]
    int getErrCode() const { return m_err_code; }

    [[nodiscard]]
    std::string getErrInfo() const { return m_err_info; }

    // 当 EventLoop 捕获到 RayrpcException 及其子类对象的异常时，会执行该函数
    virtual void handle() = 0;

protected:
    int m_err_code{0};
    std::string m_err_info;
};

}  // namespace rayrpc