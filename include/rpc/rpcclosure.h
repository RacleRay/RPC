#pragma once

#include <functional>
#include <google/protobuf/stubs/callback.h>


namespace rayrpc {
    
class RpcClosure : public google::protobuf::Closure {
public:
    RpcClosure(std::function<void(void)> callback) : m_cb(std::move(callback)) {}

    void Run() override {
        if (m_cb != nullptr) {
            m_cb();
        }
    }

private:
    std::function<void()> m_cb {nullptr};

};

}  // namespace rayrpc