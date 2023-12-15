#include "runinfo.h"


namespace rayrpc {
    
thread_local RunInfo* t_run_info = nullptr;

RunInfo* RunInfo::GetRunInfo() {
    if (t_run_info) {
        return t_run_info;
    }
    t_run_info = new RunInfo();
    return t_run_info;
}

RpcInterface* RunInfo::getRpcInterface() const noexcept {
    return m_rpc_interface;
}

}  // namespace rayrpc