#pragma once

#include <vector>

#include "tcp/tcpbuffer.h"
#include "abstractproto.h"


namespace rayrpc {
    
class AbstractCoder {
public:
    // 将 protocol message 对象转化为字节流，写入到 out_buffer 中
    virtual void encode(std::vector<AbstractProtocol::s_ptr>& protocols, TcpBuffer::s_ptr& out_buffer) = 0;

    // 将 buffer 里面字节流转换为 protocol message 对象
    virtual void decode(TcpBuffer::s_ptr& buffer, std::vector<AbstractProtocol::s_ptr>& out_protocols) = 0;

    virtual ~AbstractCoder() = default;
};

}  // namespace rayrpc