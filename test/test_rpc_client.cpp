#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <netinet/in.h>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include <google/protobuf/service.h>

#include "config.h"
#include "log.h"
#include "protocol/abstractproto.h"
#include "rpc/rpcchannel.h"
#include "rpc/rpcclosure.h"
#include "rpc/rpcdispatcher.h"
#include "tcp/netaddr.h"
#include "tcp/tcpclient.h"
#include "tcp/tcpserver.h"

#include "test.pb.h"


void test_tcp_client() {
    rayrpc::IPNetAddr::s_ptr addr = std::make_shared<rayrpc::IPNetAddr>("127.0.0.1", 12345);
    rayrpc::TcpClient client = rayrpc::TcpClient(addr);

    client.connect([addr, &client]() 
    {
        DEBUGLOG("test_tcp_client : connect to [%s] success", addr->toString().c_str());
        auto request = std::make_shared<rayrpc::TinyPBProtocol>();
        request->m_req_id = "0001";
        request->m_pb_data = "hello rayrpc.";

        testCommRequest pb_data;
        pb_data.set_count(1);
        pb_data.set_name("test");

        if (!pb_data.SerializeToString(&(request->m_pb_data))) {
            ERRLOG("test_tcp_client : serialize pb data error");
            return;
        }

        request->m_method_name = "Comm.testComm";

        client.writeMessage(request, [pb_data](rayrpc::AbstractProtocol::s_ptr msg) 
        {
            DEBUGLOG("test_tcp_client : write message success, pb data is [%s].", pb_data.ShortDebugString().c_str());
        });

        client.readMessage("0001", [](rayrpc::AbstractProtocol::s_ptr msg)
        {
            auto protomsg = std::dynamic_pointer_cast<rayrpc::TinyPBProtocol>(msg);
            DEBUGLOG("test_tcp_client : req id [%s], get reponse [%s]", protomsg->m_req_id.c_str(), protomsg->m_pb_data.c_str());

            testCommResponse pb_data;
            if (!pb_data.ParseFromString(protomsg->m_pb_data)) {
                ERRLOG("test_tcp_client : parse pb data error");
                return;
            }

            DEBUGLOG("test_tcp_client : response pb data [%s]", pb_data.ShortDebugString().c_str());
        });
    });
}


void test_rpc_channel(const std::string& ip, uint16_t port) {
    auto request = std::make_shared<testCommRequest>();
    auto response = std::make_shared<testCommResponse>();

    request->set_count(1);
    request->set_name("test");

    auto rpc_channel = std::make_shared<rayrpc::RpcChannel>(std::make_shared<rayrpc::IPNetAddr>(ip, port));

    auto rpc_controller = std::make_shared<rayrpc::RpcController>();
    rpc_controller->SetReqId("666666");
    rpc_controller->SetTimeout(10000);

    auto rpc_closure = std::make_shared<rayrpc::RpcClosure>(
        [request, response, rpc_channel, rpc_controller]() mutable {
            if (rpc_controller->GetErrorCode() != 0) {
                ERRLOG("test_rpc_channel : request [%s], rpc error code [%d], error message [%s]", 
                    request->ShortDebugString().c_str(),
                    rpc_controller->GetErrorCode(), 
                    rpc_controller->GetErrorInfo().c_str());
            } else {
                INFOLOG("test_rpc_channel : rpc success. request [%s], response [%s]", 
                    request->ShortDebugString().c_str(),
                    response->ShortDebugString().c_str());
            }

            INFOLOG("test_rpc_channel : rpc end.");
            rpc_channel->getTcpClient()->stop();
            rpc_channel.reset();  // for distructor
        }
    );

    // rpc_channel->Init(rpc_controller, request, response, rpc_closure);
    Comm_Stub stub(rpc_channel.get());
    stub.testComm(rpc_controller.get(), request.get(), response.get(), rpc_closure.get());
}


int main() {
    rayrpc::Config::setGlobalConfig("../../rayrpc_client.xml", rayrpc::ConfigType::ClientConfig);
    rayrpc::Logger::initGlobalLogger(rayrpc::LogType::Console);

    auto* config = rayrpc::Config::getGlobalConfig();
    // test_tcp_client();
    test_rpc_channel(config->m_server_ip, config->m_port);

    INFOLOG("main : test_rpc_channel end.");

    return 0;
}