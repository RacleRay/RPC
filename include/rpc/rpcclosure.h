#pragma once

#include <functional>
#include <google/protobuf/stubs/callback.h>

#include "exception.h"
#include "log.h"
#include "rpc/rpcinterface.h"
#include "runinfo.h"

namespace rayrpc {
    
class RpcClosure : public google::protobuf::Closure {
public:
    using if_s_ptr = std::shared_ptr<RpcInterface>;

    explicit RpcClosure(std::function<void(void)> callback, if_s_ptr rpc_interface) 
        : m_cb(std::move(callback)), m_rpc_interface(std::move(rpc_interface)) 
    {
        INFOLOG("RpcClosure created");
    }

    ~RpcClosure() override {
        INFOLOG("RpcClosure destroyed");
    }

    void Run() override {
        // 更新 runtime 的 RpcInterface
        // 这里在执行 Run 的时候，都会以 RpcInterface 找到对应的接口，实现打印 app 日志等
        if (!m_rpc_interface) {
            RunInfo::GetRunInfo()->m_rpc_interface = m_rpc_interface.get();
        }

        try {
            if (m_cb != nullptr) {
                m_cb();
            }
            // if (m_rpc_interface) {
            //     m_rpc_interface.reset();
            // }
        } catch (RayrpcException& e) {
            ERRLOG("RpcClosure exception: RayrpcException {}, deal with RayrpcException.handle().", e.what());
            e.handle();
            // if (m_rpc_interface) {
            //     m_rpc_interface->setError(e.getErrorCode(), e.getErrorInfo());
            //     m_rpc_interface.reset();
            // }
        } catch (std::exception& e) {
            ERRLOG("RpcClosure exception: std::exception {}.", e.what());
            // m_rpc_interface->setError(-1, e.what());
            // m_rpc_interface.reset();
        } catch (...) {
            ERRLOG("RpcClosure exception: unknown exception.");
            // m_rpc_interface->setError(-1, "unknown exception");
            // m_rpc_interface.reset();
        }
    }

private:
    std::function<void()> m_cb {nullptr};

    if_s_ptr m_rpc_interface {nullptr};
};

}  // namespace rayrpc