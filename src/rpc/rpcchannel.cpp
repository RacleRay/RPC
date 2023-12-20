#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>

#include "config.h"
#include "errorcode.h"
#include "log.h"
#include "protocol/tinypbproto.h"
#include "rpc/reqid.h"
#include "rpc/rpcchannel.h"
#include "rpc/rpcclosure.h"
#include "rpc/rpccontroller.h"
#include "runinfo.h"
#include "tcp/tcpclient.h"
#include "timer/timerevent.h"


namespace rayrpc {


// void RpcChannel::Init(controller_s_ptr controller, message_s_ptr req, message_s_ptr res, closure_s_ptr closure) {
//     if (m_is_init) {
//         return;
//     }
//     m_controller = std::move(controller);
//     m_request = std::move(req);
//     m_response = std::move(res);
//     m_closure = std::move(closure);
//     m_is_init = true;
// }


void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done)
{
    auto* closure = dynamic_cast<RpcClosure*>(done);
    auto* ctrller = dynamic_cast<RpcController*>(controller);
    if (ctrller == nullptr || request == nullptr || response == nullptr) {
        ERRLOG("RpcChannel::CallMethod : RpcController convert error.");
        ctrller->SetError(ERROR_RPC_CHANNEL_INIT, "CallMethod parameter is nullptr");
        Callback(ctrller, closure);
        return;
    }

    if (m_peer_addr == nullptr) {
        ERRLOG("RpcChannel::CallMethod : failed get peer addr.");
        ctrller->SetError(ERROR_RPC_PEER_ADDR, "peer addr is nullptr.");
        Callback(ctrller, closure);
        return;
    }
    // check valid, then make client
    m_client = std::make_shared<TcpClient>(m_peer_addr);

    // set protocol message id
    auto req_protocol = std::make_shared<TinyPBProtocol>();
    if (ctrller->GetReqId().empty()) {
        // 先从 run time info 里面取, 取不到再生成一个
        // 目的是为了实现 req_id 的透传
        // 假设服务 A 调用了 B，那么同一个 req_id 可以在服务 A 和 B 之间串起来，方便日志追踪
        std::string req_id = RunInfo::GetRunInfo()->m_req_id;
        if (!req_id.empty()) {
            req_protocol->m_req_id = req_id; 
            ctrller->SetReqId(req_id);
        } else {
            req_protocol->m_req_id = gen_request_id();
            ctrller->SetReqId(req_protocol->m_req_id);
        }
    } else {
        req_protocol->m_req_id = ctrller->GetReqId();
    }

    // set method full name
    req_protocol->m_method_name = method->full_name();
    INFOLOG("RpcChannel::CallMethod : req id [%s], method full name = [%s]", 
            req_protocol->m_req_id.c_str(),
            req_protocol->m_method_name.c_str());

    // serialize pb_data
    if (!request->SerializeToString(&req_protocol->m_pb_data)) {
        ctrller->SetError(ERROR_FAILED_SERIALIZE, "serialize pb_data failed.");
        ERRLOG("RpcChannel::CallMethod : req id [%s], serialize pb_data failed.", req_protocol->m_req_id.c_str());
        Callback(ctrller, closure);
        return;
    }

    // // init channel member for callback
    // if (!m_is_init) {
    //     ctrller->SetError(ERROR_RPC_CHANNEL_INIT, "RpcChannel not init.");
    //     ERRLOG("RpcChannel::CallMethod : req id [%s], RpcChannel not init.", req_protocol->m_req_id.c_str());
    //     return;
    // }

    // client connect to server and set closure callback
    // TcpClient client(m_peer_addr);
    auto self_channel = shared_from_this();

    // // add timeout event
    auto timer_event = std::make_shared<TimerEvent>(ctrller->GetTimeout(), false, 
        [ctrller, closure, self_channel]() mutable
        {
            INFOLOG("RpcChannel::CallMethod : [%s] rpc timeout arrive.", ctrller->GetReqId().c_str());
            // lazy reset channel, release resources.
            if (ctrller->IsFinished()) {
                self_channel.reset();
                return;
            }
            ctrller->StartCancel();
            ctrller->SetError(ERROR_RPC_CALL_TIMEOUT, "rpc call timeout " + std::to_string(ctrller->GetTimeout()));
         
            self_channel->Callback(ctrller, closure);
            self_channel.reset();
        }
    );
    m_client->addTimerEvent(timer_event);

    // set connect callback
    m_client->connect([req_protocol, self_channel, ctrller, response, closure]() mutable 
        {
            // auto* ctrller = dynamic_cast<RpcController*>(self_channel->getController());
            TcpClient* client = self_channel->getTcpClient();
            
            int err_code = client->getConnectErrorCode();
            if (err_code != 0) {
                ctrller->SetError(err_code, client->getConnectErrorInfo());
                ERRLOG("RpcChannel::CallMethod : [%s | %s] connect failed, error code = [%d], error info = [%s]",
                    req_protocol->m_req_id.c_str(), 
                    client->getPeerAddr()->toString().c_str(),
                    err_code, 
                    client->getConnectErrorInfo().c_str());
                
                Callback(ctrller, closure);
                return;
            }
            
            // success info
            INFOLOG("RpcChannel::CallMethod : [%s to %s] connect success, req id [%s].",
                client->getPeerAddr()->toString().c_str(),
                client->getLocalAddr()->toString().c_str(),
                req_protocol->m_req_id.c_str()
            );

            self_channel->getTcpClient()->writeMessage(req_protocol, [req_protocol, self_channel, ctrller, response, closure](AbstractProtocol::s_ptr) mutable 
            {
                INFOLOG("RpcChannel::CallMethod : %s send rpc request success. call method name [%s]", 
                        req_protocol->m_req_id.c_str(), 
                        req_protocol->m_method_name.c_str());

                self_channel->getTcpClient()->readMessage(req_protocol->m_req_id, [self_channel, ctrller, response, closure](AbstractProtocol::s_ptr msg) mutable 
                {
                    // parse message from server
                    auto rsp_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(msg);
                    INFOLOG("RpcChannel::CallMethod : %s success get rpc response, call method name [%s]", 
                            rsp_protocol->m_req_id.c_str(), rsp_protocol->m_method_name.c_str());

                    // remove timer event  ！Note ：will do these at Callback
                    // self_channel->getTimerEvent()->setCanceled(true);
                    // ctrller->SetFinished(true);  

                    // deserialize
                    // auto* ctrller = dynamic_cast<RpcController*>(self_channel->getController());
                    if (!(response->ParseFromString(rsp_protocol->m_pb_data))) {
                        ERRLOG("RpcChannel::CallMethod : %s parse pb_data failed.", rsp_protocol->m_req_id.c_str());
                        ctrller->SetError(ERROR_FAILED_DESERIALIZE, "parse pb_data failed.");
                        Callback(ctrller, closure);
                        return;
                    }     

                    // check error info
                    if (rsp_protocol->m_err_code != 0) {
                        ERRLOG("RpcChannel::CallMethod : %s rpc method [%s], get rpc response error, error code = [%d], error msg = [%s]",
                                rsp_protocol->m_req_id.c_str(), rsp_protocol->m_method_name.c_str(), 
                                rsp_protocol->m_err_code, rsp_protocol->m_err_info.c_str());
                        ctrller->SetError(rsp_protocol->m_err_code, std::move(rsp_protocol->m_err_info));
                        Callback(ctrller, closure);
                        return;
                    }

                    INFOLOG("RpcChannel::CallMethod : %s call rpc method [%s], get rpc response success.", 
                        rsp_protocol->m_req_id.c_str(), rsp_protocol->m_method_name.c_str());

                    if (!ctrller->IsCanceled() && closure != nullptr) {
                        closure->Run();
                    }
                    
                    Callback(ctrller, closure);
                });
        });
    });
}


// run closure and set task finished, then wait to recycle.
inline void RpcChannel::Callback(RpcController* controller, RpcClosure* closure) {
    if (controller->IsFinished()) {
        return;
    }
    if (closure) {
        closure->Run();
        if (controller) {
            controller->SetFinished(true);
        }
    }
}


NetAddr::s_ptr RpcChannel::FindAddr(const std::string& str) {
    if (IPNetAddr::CheckValid(str)) {
        return std::make_shared<IPNetAddr>(str);
    }

    // find from stubs
    auto it = Config::getGlobalConfig()->m_rpc_stubs.find(str);
    if (it != Config::getGlobalConfig()->m_rpc_stubs.end()) {
        INFOLOG("RpcChannel::FindAddr : find stub [%s] addr [%s].", str.c_str(), (*it).second.addr->toString().c_str());
        return (*it).second.addr;
    }

    INFOLOG("RpcChannel::FindAddr : can`t find stub [%s] addr.", str.c_str());
    return nullptr;
}


// google::protobuf::RpcController* RpcChannel::getController() {
//     return m_controller.get();
// }

// google::protobuf::Message* RpcChannel::getRequest() {
//      return m_request.get();
// }

// google::protobuf::Message* RpcChannel::getResponse() {
//     return m_response.get();
// }

// google::protobuf::Closure* RpcChannel::getClosure() {
//     return m_closure.get();
// }

TcpClient* RpcChannel::getTcpClient() {
    return m_client.get();
}

// TimerEvent::s_ptr RpcChannel::getTimerEvent() {
//     return m_timer_event;
// }

}  // namespace rayrpc