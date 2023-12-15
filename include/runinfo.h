#pragma once

#include <string>

namespace rayrpc {
    
class RunInfo {
public:
    static RunInfo* GetRunInfo();

public:
    std::string m_req_id;
    std::string m_method_name;
};

}  // namespace rayrpc