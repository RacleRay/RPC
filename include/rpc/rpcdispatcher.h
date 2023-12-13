#pragma once

#include <google/protobuf/service.h>
#include <map>
#include <memory>

#include "protocol/abstractproto.h"
#include "protocol/tinypbproto.h"
#include "tcp/tcpconnection.h"


namespace rayrpc {
    
class RpcDispatcher {
public:
    using service_s_ptr = std::shared_ptr<google::protobuf::Service>;

    // process request and generate response
    void dispatch(const AbstractProtocol::s_ptr& request, const AbstractProtocol::s_ptr& response, const TcpConnection::s_ptr& conn);

    void registerService(service_s_ptr service);

    static void setTinyPBError(const TinyPBProtocol::s_ptr&  msg, int32_t err_code, std::string&& err_info);

public:
    static RpcDispatcher* GetRpcDispatcher();

private:
    // get service name and method name from full name
    static bool ParseServiceFullName(const std::string& full_name, std::string& service_name, std::string& method_name);

private:
    std::map<std::string, service_s_ptr> m_service_map;
};

}  // namespace rayrpc