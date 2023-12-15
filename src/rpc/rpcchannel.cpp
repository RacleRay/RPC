#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>

#include "errorcode.h"
#include "log.h"
#include "protocol/tinypbproto.h"
#include "rpc/reqid.h"
#include "rpc/rpcchannel.h"
#include "rpc/rpccontroller.h"
#include "tcp/tcpclient.h"
#include "timer/timerevent.h"


namespace rayrpc {


// void RpcChannel::Init(controller_s_ptr controller, message_s_ptr req, message_s_ptr res, closure_s_ptr done) {
//     if (m_is_init) {
//         return;
//     }
//     m_controller = std::move(controller);
//     m_request = std::move(req);
//     m_response = std::move(res);
//     m_closure = std::move(done);
//     m_is_init = true;
// }


void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done)
{
    auto req_protocol = std::make_shared<TinyPBProtocol>();
    auto* ctrller = dynamic_cast<RpcController*>(controller);
    if (ctrller == nullptr) {
        ERRLOG("RpcChannel::CallMethod : RpcController convert error.");
    }

    // set protocol message id
    if (ctrller->GetReqId().empty()) {
        req_protocol->m_req_id = gen_request_id();
        ctrller->SetReqId(req_protocol->m_req_id);
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
        return;
    }

    // // init channel member for callback
    // if (!m_is_init) {
    //     ctrller->SetError(ERROR_RPC_CHANNEL_INIT, "RpcChannel not init.");
    //     ERRLOG("RpcChannel::CallMethod : req id [%s], RpcChannel not init.", req_protocol->m_req_id.c_str());
    //     return;
    // }

    // client connect to server and set done callback
    // TcpClient client(m_peer_addr);
    auto self_channel = shared_from_this();

    // // add timeout event
    m_timer_event = std::make_shared<TimerEvent>(ctrller->GetTimeout(), false, 
        [ctrller, done]() mutable
        {
            ctrller->StartCancel();
            ctrller->SetError(ERROR_RPC_CALL_TIMEOUT, "rpc call timeout " + std::to_string(ctrller->GetTimeout()));
            if (done) {
                done->Run();
            }
        }
    );
    m_client->addTimerEvent(m_timer_event);

    // set connect callback
    m_client->connect([req_protocol, self_channel, ctrller, response, done]() mutable 
        {
            // auto* ctrller = dynamic_cast<RpcController*>(self_channel->getController());
            if (self_channel->getTcpClient()->getConnectErrorCode() != 0) {

            }

            self_channel->getTcpClient()->writeMessage(req_protocol, [req_protocol, self_channel, ctrller, response, done](AbstractProtocol::s_ptr) mutable 
            {
                INFOLOG("RpcChannel::CallMethod : %s send rpc request success. call method name [%s]", 
                        req_protocol->m_req_id.c_str(), req_protocol->m_method_name.c_str());

                self_channel->getTcpClient()->readMessage(req_protocol->m_req_id, [self_channel, ctrller, response, done](AbstractProtocol::s_ptr msg) mutable 
                {   
                    // parse message from server
                    auto rsp_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(msg);
                    INFOLOG("RpcChannel::CallMethod : %s success get rpc response, call method name [%s]", 
                            rsp_protocol->m_req_id.c_str(), rsp_protocol->m_method_name.c_str());

                    // remove timer event
                    self_channel->getTimerEvent()->setCanceled(true);

                    // deserialize
                    // auto* ctrller = dynamic_cast<RpcController*>(self_channel->getController());
                    if (!(response->ParseFromString(rsp_protocol->m_pb_data))) {
                        ERRLOG("RpcChannel::CallMethod : %s parse pb_data failed.", rsp_protocol->m_req_id.c_str());
                        ctrller->SetError(ERROR_FAILED_DESERIALIZE, "parse pb_data failed.");
                        return;
                    }     

                    // check error info
                    if (rsp_protocol->m_err_code != 0) {
                        ERRLOG("RpcChannel::CallMethod : %s rpc method [%s], get rpc response error, error code = [%d], error msg = [%s]",
                                rsp_protocol->m_req_id.c_str(), rsp_protocol->m_method_name.c_str(), 
                                rsp_protocol->m_err_code, rsp_protocol->m_err_info.c_str());
                        ctrller->SetError(rsp_protocol->m_err_code, std::move(rsp_protocol->m_err_info));
                        return;
                    }

                    if (!ctrller->IsCanceled() && done != nullptr) {
                        done->Run();
                    }

                    self_channel.reset();
                });
        });
    });
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

TimerEvent::s_ptr RpcChannel::getTimerEvent() {
    return m_timer_event;
}

}  // namespace rayrpc