#pragma once 

#include "abstractcoder.h"
#include "tinypbproto.h"


namespace rayrpc {
    
class TinyPBCoder: public AbstractCoder {

public:
    TinyPBCoder() = default;
    ~TinyPBCoder() override = default;

    // 将 protocol message 对象转化为字节流，写入到 out_buffer 中
    void encode(std::vector<AbstractProtocol::s_ptr>& protomsg, TcpBuffer::s_ptr& out_buffer) override;

    // 将 buffer 里面字节流转换为 protocol message 对象
    void decode(TcpBuffer::s_ptr& buffer, std::vector<AbstractProtocol::s_ptr>& out_protomsg) override;

private:
    
};

}  // namespace rayrpc