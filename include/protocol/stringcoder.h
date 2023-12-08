#pragma once

#include "abstractcoder.h"
#include "abstractproto.h"

namespace rayrpc {
    
class StringProtocol : public AbstractProtocol {
public:
    std::string info;
};

class StringCoder : public AbstractCoder {
public:
    // 将 protocol message 对象转化为字节流，写入到 out_buffer 中
    void encode(std::vector<AbstractProtocol::s_ptr>& protocols, TcpBuffer::s_ptr& out_buffer) override {
        for (auto& proto_msg : protocols) {
            std::shared_ptr<StringProtocol> tmp = std::dynamic_pointer_cast<StringProtocol>(proto_msg);
            // out_buffer->writeToBuffer(tmp->info.c_str(), tmp->info.size() + 1);  // +1 为了处理 \0(null char)
            out_buffer->writeToBuffer(tmp->info.c_str(), tmp->info.size());
        }
    }

    // 将 buffer 里面字节流转换为 protocol message 对象
    void decode(TcpBuffer::s_ptr& buffer, std::vector<AbstractProtocol::s_ptr>& out_protocols) override {
        std::vector<char> tmp_buffer;
        buffer->readFromBuffer(tmp_buffer, buffer->readAble());

        std::shared_ptr<StringProtocol> msg = std::make_shared<StringProtocol>();
        msg->info = std::string(tmp_buffer.begin(), tmp_buffer.end());
        msg->setReqId("001");

        out_protocols.push_back(msg);
    }
};

}  // namespace rayrpc