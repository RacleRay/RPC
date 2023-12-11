#include "protocol/tinypbproto.h"


namespace rayrpc {
    
u_char TinyPBProtocol::PB_START = 0xff;
u_char TinyPBProtocol::PB_END = 0xfe;

// 计算大小，别算漏了，BUG FIXED
size_t TinyPBProtocol::BYTE_SIZE_FIXED = sizeof(PB_START) + sizeof(PB_END) + sizeof(m_pkg_len)
                            + sizeof(m_err_info_len) + sizeof(m_req_id_len) + sizeof(m_method_name_len)
                            + sizeof(m_err_code) + sizeof(m_check_sum);

}  // namespace rayrpc