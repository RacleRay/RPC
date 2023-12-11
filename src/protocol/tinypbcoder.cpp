#include <arpa/inet.h>
#include <cstring>
#include <vector>

#include "log.h"
#include "protocol/tinypbcoder.h"
#include "protocol/tinypbproto.h"
#include "utils.h"


namespace {
    
inline int find_pb_start(rayrpc::TcpBuffer::s_ptr &buffer, bool& proto_end, size_t& out_start_index, size_t& out_end_index) {
    int pkg_len = 0;

    const std::vector<char>& mbuf = buffer->m_buffer;
    size_t i = 0;
    for (i = out_start_index; i < buffer->writeIndex(); i++) {
        if (memcmp(&mbuf[i], &rayrpc::TinyPBProtocol::PB_START, 1) == 0) {
            if (i + 4 < buffer->writeIndex()) {
                pkg_len = rayrpc::details::get_int32_from_netbytes(&mbuf[i + 1]);
                DEBUGLOG("TinyPBCoder::decode : receive package len = [%d].", pkg_len);

                size_t j = i + pkg_len - 1;
                if (j >= buffer->writeIndex()) { 
                    continue;
                }
                // if (mbuf[j] == rayrpc::TinyPBProtocol::PB_END) {
                if (memcmp(&mbuf[j], &rayrpc::TinyPBProtocol::PB_END, 1) == 0) {
                    out_start_index = i;
                    out_end_index = j;
                    buffer->moveReadIndex(out_end_index - out_start_index + 1);
                    break;
                }
            }
        }
    }

    if (i >= buffer->writeIndex()) {
        DEBUGLOG("TinyPBCoder::decode : decode end, no message found.");
        proto_end = true;
    }

    return pkg_len;
}


inline bool parse_req_id(const std::vector<char>& mbuf, std::shared_ptr<rayrpc::TinyPBProtocol> &proto_msg, size_t& start_index, size_t end_index) {
    size_t req_id_len_index = start_index + sizeof(char) + sizeof(proto_msg->m_pkg_len);
    if (req_id_len_index >= end_index) {
        proto_msg->parse_success = false;
        ERRLOG("TinyPBCoder::decode : parse req id len failed, req_id_len_index[%ld] >= end_index[%ld].", req_id_len_index, end_index);
        return false;
    }
    proto_msg->m_req_id_len = rayrpc::details::get_int32_from_netbytes(&mbuf[req_id_len_index]);
    DEBUGLOG("TinyPBCoder::decode : parse req id len = [%d].", proto_msg->m_req_id_len);

    // parse request id
    size_t req_id_idx = req_id_len_index + sizeof(proto_msg->m_req_id_len);
    if (req_id_idx >= end_index) {
        proto_msg->parse_success = false;
        ERRLOG("TinyPBCoder::decode : parse req id failed, req_id_idx[%ld] >= end_index[%ld].", req_id_idx, end_index);
        return false;
    }

    char req_id[64] = {0};
    memcpy(&req_id[0], &mbuf[req_id_idx], proto_msg->m_req_id_len);
    proto_msg->m_req_id = std::move(std::string(req_id));
    DEBUGLOG("TinyPBCoder::decode : parse req_id = [%s].", proto_msg->m_req_id.c_str());

    start_index = req_id_idx + proto_msg->m_req_id_len;

    return true;
}


inline bool parse_method_name(const std::vector<char>& mbuf, std::shared_ptr<rayrpc::TinyPBProtocol> &proto_msg, size_t& start_index, size_t end_index) {
    size_t method_name_len_index = start_index;
    if (method_name_len_index >= end_index) {
        proto_msg->parse_success = false;
        ERRLOG("TinyPBCoder::decode : parse method_name len failed, method_name_len_index[%ld] >= end_index[%ld].", method_name_len_index, end_index);
        return false;
    }
    proto_msg->m_method_name_len = rayrpc::details::get_int32_from_netbytes(&mbuf[method_name_len_index]);

    size_t method_name_index = method_name_len_index + sizeof(proto_msg->m_method_name_len);
    if (method_name_index >= end_index) {
        proto_msg->parse_success = false;
        ERRLOG("TinyPBCoder::decode : parse method_name failed, method_name_index[%ld] >= end_index[%ld].", method_name_index, end_index);
        return false;
    }

    char method_name[512] = {0};
    memcpy(&method_name[0], &mbuf[method_name_index], proto_msg->m_method_name_len);
    proto_msg->m_method_name = std::move(std::string(method_name));
    DEBUGLOG("parse method_name=%s", proto_msg->m_method_name.c_str());

    start_index = method_name_index + proto_msg->m_method_name_len;

    return true;
}


inline bool parse_error_info(const std::vector<char>& mbuf, std::shared_ptr<rayrpc::TinyPBProtocol> &proto_msg, size_t& start_index, size_t end_index) {
    size_t err_code_idx = start_index;
    if (err_code_idx >= end_index) {
        proto_msg->parse_success = false;
        ERRLOG("TinyPBCoder::decode : parse error code failed, err_code_idx[%ld] >= end_index[%ld].", err_code_idx, end_index);
        return false;
    }
    proto_msg->m_err_code = rayrpc::details::get_int32_from_netbytes(&mbuf[err_code_idx]);
    DEBUGLOG("TinyPBCoder::decode : parse error code = [%d].", proto_msg->m_err_code);

    size_t err_info_len_idx = err_code_idx + sizeof(proto_msg->m_err_code);
    if (err_info_len_idx >= end_index) {
        proto_msg->parse_success = false;
        ERRLOG("TinyPBCoder::decode : parse error info len failed, err_info_len_idx[%ld] >= end_index[%ld].", err_info_len_idx, end_index);
        return false;
    }
    proto_msg->m_err_info_len = rayrpc::details::get_int32_from_netbytes(&mbuf[err_info_len_idx]);
    DEBUGLOG("TinyPBCoder::decode : parse error info len = [%d].", proto_msg->m_err_info_len);

    size_t err_info_idx = err_info_len_idx + sizeof(proto_msg->m_err_info_len);
    if (err_info_idx >= end_index) {
        proto_msg->parse_success = false;
        ERRLOG("TinyPBCoder::decode : parse error info failed, err_info_idx[%ld] >= end_index[%ld].", err_info_idx, end_index);
        return false;
    }
    char err_info[512] = {0};
    memcpy(&err_info[0], &mbuf[err_info_idx], proto_msg->m_err_info_len);
    proto_msg->m_err_info = std::move(std::string(err_info));
    DEBUGLOG("TinyPBCoder::decode : parse error info = [%s].", proto_msg->m_err_info.c_str());

    start_index = err_info_idx + proto_msg->m_err_info_len;

    return true;
}


inline char* encode_int32(char *buf, int32_t val) {
    int32_t val_net = htonl(val);
    memcpy(buf, &val_net, sizeof(val_net));
    DEBUGLOG("encode_int32 : val = [%d], val_net = [%d].", val, val_net);
    buf += sizeof(val_net);
    return buf;
}


inline char* encode_string(char *buf, const std::string& str, size_t len) {
    if (!str.empty()) {
        memcpy(buf, &(str[0]), len);
        DEBUGLOG("encode_string : str = [%s], len = [%d].", str.c_str(), len);
        buf += len;
    }
    return buf;
}


inline char* encode_len_and_data(char *buf, const std::string& data) {
    size_t data_len = data.length();
    buf = encode_int32(buf, (int32_t)data_len);

    buf = encode_string(buf, data, data_len);

    return buf;
}


}  // namespace 


namespace rayrpc {

void TinyPBCoder::encode(std::vector<AbstractProtocol::s_ptr> &protomsg, TcpBuffer::s_ptr &out_buffer) {
    for (auto& msg : protomsg) {
        std::shared_ptr<TinyPBProtocol> pmsg = std::dynamic_pointer_cast<TinyPBProtocol>(msg);

        if (pmsg->m_req_id.empty()) {
            pmsg->m_req_id = "001";
        }
        DEBUGLOG("TinyPBCoder::encode : req_id = [%s].", pmsg->m_req_id.c_str());

        size_t pkg_len = TinyPBProtocol::BYTE_SIZE_FIXED + pmsg->m_req_id.length() + \
                            pmsg->m_method_name.length() + pmsg->m_pb_data.length() + pmsg->m_err_info.length();        

        char* buf = reinterpret_cast<char *>(malloc(pkg_len));
        char* tmp = buf;

        // begin encode
        memcpy(tmp, &TinyPBProtocol::PB_START, 1);
        tmp++;

        tmp = encode_int32(tmp, (int32_t)pkg_len);

        tmp = encode_len_and_data(tmp, pmsg->m_req_id);

        tmp = encode_len_and_data(tmp, pmsg->m_method_name);

        tmp = encode_int32(tmp, pmsg->m_err_code);

        tmp = encode_len_and_data(tmp, pmsg->m_err_info);

        tmp = encode_string(tmp, pmsg->m_pb_data, pmsg->m_pb_data.length());

        // TODO: check sum needed
        // int32_t checksum = checksum(buf, pkg_len);
        tmp = encode_int32(tmp, 1);  // checksum

        memcpy(tmp, &TinyPBProtocol::PB_END, 1);
        tmp++;

        DEBUGLOG("TinyPBCoder::encode : encode message success, req_id [%s], pkg_len[%d].", pmsg->m_req_id.c_str(), pkg_len);
        // DEBUGLOG("TinyPBCoder::encode : message = [%s].", buf);
        // end encode

        if (buf != nullptr && pkg_len != 0) {
            out_buffer->writeToBuffer(buf, pkg_len);
        }
        if (buf) {
            free(const_cast<char*>(buf));
            buf = nullptr;
        }
    }
}

// 将 buffer 里面字节流转换为 protocol message 对象
void TinyPBCoder::decode(TcpBuffer::s_ptr &buffer, std::vector<AbstractProtocol::s_ptr> &out_protomsg) {
    bool proto_end = false;
    while (!proto_end) {
        // 1. 找到 PB_START
        size_t start_index = buffer->readIndex();
        size_t end_index = 0;
        int pkg_len = find_pb_start(buffer, proto_end, start_index, end_index);
        if (pkg_len == 0) {
            continue;
        }

        // 2. 解析 PB_START 后的数据
        std::shared_ptr<TinyPBProtocol> proto_msg = std::make_shared<TinyPBProtocol>();
        proto_msg->m_pkg_len = pkg_len;

        if (!parse_req_id(buffer->m_buffer, proto_msg, start_index, end_index)) {
            continue;
        }

        if (!parse_method_name(buffer->m_buffer, proto_msg, start_index, end_index)) {
            continue;
        }

        if (!parse_error_info(buffer->m_buffer, proto_msg, start_index, end_index)) {
            continue;
        }

        size_t pb_data_len = proto_msg->m_pkg_len - proto_msg->m_method_name_len - proto_msg->m_req_id_len 
                            - proto_msg->m_err_info_len - TinyPBProtocol::BYTE_SIZE_FIXED;
        proto_msg->m_pb_data = std::move(std::string(&buffer->m_buffer[start_index], pb_data_len));

        // check sum skipped

        proto_msg->parse_success = true;

        out_protomsg.push_back(proto_msg);
    }
}

} // namespace rayrpc