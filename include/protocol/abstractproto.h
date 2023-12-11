
#pragma once

#include <memory>
#include <string>

namespace rayrpc {

class AbstractProtocol : public std::enable_shared_from_this<AbstractProtocol> {
  public:
    using s_ptr = std::shared_ptr<AbstractProtocol>;

    std::string getReqId() const {
        return m_req_id;
    }

    void setReqId(const std::string &req_id) {
        m_req_id = req_id;
    }

    virtual ~AbstractProtocol() = default;

    std::string m_req_id;
};

} // namespace rayrpc