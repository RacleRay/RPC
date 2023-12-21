#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>

#include "errorcode.h"
#include "log.h"
#include "rpc/rpcclosure.h"
#include "rpc/rpccontroller.h"
#include "rpc/rpcdispatcher.h"
#include "runinfo.h"
#include "tcp/netaddr.h"
#include "tcp/tcpconnection.h"


namespace {
    
rayrpc::RpcDispatcher* g_rpc_dispatcher = nullptr;

#define DELETE_RESOURCE(RESO)     \
    if ((RESO) != NULL) {         \
        delete (RESO);            \
        (RESO) = NULL;            \
    }                            \

}  // namespace


namespace rayrpc {

// process request and generate response
void RpcDispatcher::dispatch(const AbstractProtocol::s_ptr& request, const AbstractProtocol::s_ptr& response,  const TcpConnection::s_ptr& conn) {
    std::shared_ptr<TinyPBProtocol> req_proto = std::dynamic_pointer_cast<TinyPBProtocol>(request);
    std::shared_ptr<TinyPBProtocol> rsp_proto = std::dynamic_pointer_cast<TinyPBProtocol>(response);

    rsp_proto->m_req_id = req_proto->m_req_id;
    rsp_proto->m_method_name = req_proto->m_method_name;

    // parse service name and method name from full name
    std::string service_name, method_name;
    if (!ParseServiceFullName(req_proto->m_method_name, service_name, method_name)) {
        ERRLOG("RpcDispatcher::dispatch: req_id [%s], parse service name error", req_proto->m_req_id.c_str());
        setTinyPBError(rsp_proto, ERROR_PARSE_SERVICE_NAME, "parse service name error");
        return;
    }

    // find service
    auto it = m_service_map.find(service_name);
    if (it == m_service_map.end()) {
        ERRLOG("RpcDispatcher::dispatch: req_id [%s], service {%s} not found", 
            req_proto->m_req_id.c_str(), 
            service_name.c_str());
        setTinyPBError(rsp_proto, ERROR_SERVICE_NOT_FOUND, "service not found");
        return;
    }

    // deserialize the pb_data
    service_s_ptr service = it->second;
    const auto* method = service->GetDescriptor()->FindMethodByName(method_name);
    if (method == nullptr) {
        ERRLOG("RpcDispatcher::dispatch: req_id [%s], method {%s} not found", 
            req_proto->m_req_id.c_str(), 
            method_name.c_str());
        setTinyPBError(rsp_proto, ERROR_METHOD_NOT_FOUND, "method not found");
        return;
    }
    
    google::protobuf::Message* request_msg = service->GetRequestPrototype(method).New();
    if (!request_msg->ParseFromString(req_proto->m_pb_data)) {
        ERRLOG("RpcDispatcher::dispatch: req_id [%s], deserialize pb_data error", req_proto->m_req_id.c_str());
        setTinyPBError(rsp_proto, ERROR_FAILED_DESERIALIZE, "deserialize pb_data error");
        DELETE_RESOURCE(request_msg);
        return;
    }
    
    // run service method
    INFOLOG("RpcDispatcher::dispatch: req_id [%s], get RPC request {%s}", req_proto->m_req_id.c_str(), request_msg->ShortDebugString().c_str());

    google::protobuf::Message* response_msg = service->GetResponsePrototype(method).New();
    auto* rpc_ctl = new RpcController();
    rpc_ctl->SetLocalAddr(conn->getLocalAddr());
    rpc_ctl->SetPeerAddr(conn->getPeerAddr());
    rpc_ctl->SetRequestId(req_proto->m_req_id);

    // record run time information
    RunInfo::GetRunInfo()->m_req_id = req_proto->m_req_id;
    RunInfo::GetRunInfo()->m_method_name = method_name;
    // service->CallMethod(method, rpc_ctl, request_msg, response_msg, nullptr);

    auto* closure = new RpcClosure(
        [request_msg, response_msg, req_proto, rsp_proto, conn,  rpc_ctl, this] () mutable
        {
            // serialize the response data to protobuf
            if (!response_msg->SerializeToString(&(rsp_proto->m_pb_data))) {
                ERRLOG("RpcDispatcher::dispatch: req_id [%s], serialize response error, origin message [%s]", 
                    req_proto->m_req_id.c_str(), 
                    response_msg->ShortDebugString().c_str());
                setTinyPBError(rsp_proto, ERROR_FAILED_SERIALIZE, "serialize response error");
            } else {
                rsp_proto->m_err_code = 0;
                rsp_proto->m_err_info = "";
                INFOLOG("RpcDispatcher::dispatch: req_id [%s], dispatch RPC request {%s}, get RPC response {%s}", 
                        req_proto->m_req_id.c_str(), 
                        request_msg->ShortDebugString().c_str(), 
                        response_msg->ShortDebugString().c_str());
            }

            // response to client
            std::vector<AbstractProtocol::s_ptr> reply_msgs;
            reply_msgs.emplace_back(rsp_proto);
            // encode to out buffer and trigger out event.
            conn->reply(reply_msgs);  // TcpConnection : reply

            DELETE_RESOURCE(response_msg);
            DELETE_RESOURCE(request_msg);
            DELETE_RESOURCE(rpc_ctl);
        }, 
        nullptr);  // rpc_inferface is not implememted.
    
    service->CallMethod(method, rpc_ctl, request_msg, response_msg, closure);
}   

void RpcDispatcher::registerService(service_s_ptr service) {
    std::string service_name = service->GetDescriptor()->full_name();
    m_service_map[service_name] = std::move(service);
}

// ==============================================================================
// static

RpcDispatcher* RpcDispatcher::GetRpcDispatcher() {
    if (g_rpc_dispatcher != nullptr) {
        return g_rpc_dispatcher;
    }
    g_rpc_dispatcher = new RpcDispatcher();
    return g_rpc_dispatcher;
}


void RpcDispatcher::setTinyPBError(const TinyPBProtocol::s_ptr& msg, int32_t err_code, std::string &&err_info) {
    msg->m_err_code = err_code;
    msg->m_err_info = std::move(err_info);
    msg->m_err_info_len = (int32_t)err_info.length();
}

// get service name and method name from full name
// full_name format : service_name.method_name
bool RpcDispatcher::ParseServiceFullName(const std::string &full_name, std::string &service_name, std::string &method_name) {
    if (full_name.empty()) {
        ERRLOG("RpcDispatcher::ParseServiceFullName: full_name is empty");
        return false;
    }

    size_t dot_idx = full_name.find_first_of('.');
    if (dot_idx == std::string::npos) {
        ERRLOG("RpcDispatcher::ParseServiceFullName: full_name format error, [.] is not in full_name");
        return false;
    }

    service_name = full_name.substr(0, dot_idx);
    method_name = full_name.substr(dot_idx + 1, full_name.length() - 1 - dot_idx);

    INFOLOG("RpcDispatcher::ParseServiceFullName: full_name = {%s}, service_name = {%s}, method_name = {%s}", full_name.c_str(), service_name.c_str(), method_name.c_str());

    return true;
}


} // namespace rayrpc
