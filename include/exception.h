#pragma once

#include <exception>


namespace rayrpc {
    
class RayrpcException : public std::exception {
public:
    RayrpcException() = default;

    ~RayrpcException() override = default;

    // 当 EventLoop 捕获到 RayrpcException 及其子类对象的异常时，会执行该函数
    virtual void handle() = 0;
};

}  // namespace rayrpc