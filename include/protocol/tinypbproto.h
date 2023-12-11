#pragma once

#include <string>

#include "protocol/abstractproto.h"


namespace rayrpc {

/**
 * @brief
    ------------------------------------------------------------------------------------------------------------------------------------------------
    0xff | pack len | request id len | request id | rpc method len | rpc method | error code | error msg len | error msg | protobuf data | check sum | 0xef
    ------------------------------------------------------------------------------------------------------------------------------------------------
 */

struct TinyPBProtocol : public AbstractProtocol {
  public:
    TinyPBProtocol() = default;
    ~TinyPBProtocol() override = default;

    static u_char PB_START;
    static u_char PB_END;
    static size_t BYTE_SIZE_FIXED;

  public:
    int32_t m_pkg_len{0};

    int32_t m_req_id_len{0};
    // m_req_id from AbstractProtocol

    int32_t m_method_name_len{0};
    std::string m_method_name;
    
    int32_t m_err_code{0};
    int32_t m_err_info_len{0};
    std::string m_err_info;

    std::string m_pb_data;
    
    int32_t m_check_sum{0};

    bool parse_success{false};
};

} // namespace rayrpc