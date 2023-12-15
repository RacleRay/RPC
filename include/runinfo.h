#pragma once

#include <string>

namespace rayrpc {

class RpcInterface;
    
class RunInfo {
public:
    RpcInterface* getRpcInterface() const noexcept;

public:
    static RunInfo* GetRunInfo();

public:
    std::string m_req_id;
    std::string m_method_name;

    RpcInterface* m_rpc_interface {nullptr};
};

}  // namespace rayrpc