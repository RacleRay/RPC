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
#include "rpc/rpcdispatcher.h"
#include "tcp/netaddr.h"
#include "tcp/tcpclient.h"
#include "tcp/tcpserver.h"

#include "test.pb.h"


void test_tcp_client() {
    rayrpc::IPNetAddr::s_ptr addr = std::make_shared<rayrpc::IPNetAddr>("127.0.0.1", 12345);
    rayrpc::TcpClient client = rayrpc::TcpClient(addr);

    client.connect([addr, &client]() {
        DEBUGLOG("test_tcp_client : connect to [%s] success", addr->toString().c_str());
        std::shared_ptr<rayrpc::TinyPBProtocol> request = std::make_shared<rayrpc::TinyPBProtocol>();
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

        client.writeMessage(request, [pb_data](rayrpc::AbstractProtocol::s_ptr msg) {
            DEBUGLOG("test_tcp_client : write message success, pb data is [%s].", pb_data.ShortDebugString().c_str());
        });

        client.readMessage("0001", [](rayrpc::AbstractProtocol::s_ptr msg){
            std::shared_ptr<rayrpc::TinyPBProtocol> protomsg = std::dynamic_pointer_cast<rayrpc::TinyPBProtocol>(msg);
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


int main() {
    rayrpc::Config::setGlobalConfig("../../rayrpc.xml");
    rayrpc::Logger::initGlobalLogger();

    test_tcp_client();

    return 0;
}