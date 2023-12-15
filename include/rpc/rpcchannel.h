#pragma once

#include <google/protobuf/service.h>

#include "errorcode.h"
#include "log.h"
#include "protocol/tinypbproto.h"
#include "rpc/reqid.h"
#include "rpc/rpcchannel.h"
#include "rpc/rpccontroller.h"
#include "tcp/netaddr.h"
#include "tcp/tcpclient.h"


namespace rayrpc {
    
// An RpcChannel represents a communication line to a Service which can be used to call that Service's
// methods.  The Service may be running on another machine.  Normally, you
// should not call an RpcChannel directly, but instead construct a stub Service
// wrapping it.  Example:
//   RpcChannel* channel = new MyRpcChannel("remotehost.example.com:1234");
//   MyService* service = new MyService::Stub(channel);
//   service->MyMethod(request, &response, callback);
class RpcChannel : public google::protobuf::RpcChannel, public std::enable_shared_from_this<RpcChannel>  {
public:
    using s_ptr = std::shared_ptr<RpcChannel>;
    // using controller_s_ptr = std::shared_ptr<google::protobuf::RpcController>;
    // using message_s_ptr = std::shared_ptr<google::protobuf::Message>;
    // using closure_s_ptr = std::shared_ptr<google::protobuf::Closure>;

    RpcChannel() = delete;
    
    explicit RpcChannel(NetAddr::s_ptr peer_addr) : m_peer_addr(std::move(peer_addr)) {
        m_client = std::make_shared<TcpClient>(m_peer_addr);
    };
    
    ~RpcChannel() override {
        DEBUGLOG("~RpcChannel");
    }

    // void Init(controller_s_ptr controller, message_s_ptr req, message_s_ptr res, closure_s_ptr done);

    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done) override;

    // google::protobuf::RpcController* getController();

    // google::protobuf::Message* getRequest();

    // google::protobuf::Message* getResponse();

    // google::protobuf::Closure* getClosure();

    TcpClient* getTcpClient();

    TimerEvent::s_ptr getTimerEvent();

private:
    NetAddr::s_ptr m_peer_addr {nullptr};
    NetAddr::s_ptr m_local_addr {nullptr};

    TcpClient::s_ptr m_client {nullptr};

    // controller_s_ptr m_controller {nullptr};
    // message_s_ptr m_request {nullptr};
    // message_s_ptr m_response {nullptr};
    // closure_s_ptr m_closure {nullptr};

    // bool m_is_init {false};

    TimerEvent::s_ptr m_timer_event {nullptr};
};  // class RpcChannel

}  // namespace rayrpc