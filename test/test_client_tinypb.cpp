#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <netinet/in.h>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "log.h"
#include "protocol/abstractproto.h"
#include "protocol/tinypbcoder.h"
#include "protocol/tinypbproto.h"
#include "tcp/netaddr.h"
#include "tcp/tcpclient.h"
#include <arpa/inet.h>


// echo 测试 
// 应该返回  hello rayrpc! 
void test_tcpclient_tinypbproto() {
    rayrpc::IPNetAddr::s_ptr addr = std::make_shared<rayrpc::IPNetAddr>("127.0.0.1", 12345);
    rayrpc::TcpClient client(addr);

    client.connect([addr, &client]() {
        DEBUGLOG("test_tcpclient_tinypbproto : connect to [%s] success.", addr->toString().c_str());

        std::shared_ptr<rayrpc::TinyPBProtocol> protomsg = std::make_shared<rayrpc::TinyPBProtocol>();
        protomsg->m_pb_data = "test : hello rayrpc!";
        protomsg->setReqId("001");

        client.writeMessage(protomsg, [](rayrpc::AbstractProtocol::s_ptr protomsg) {
            DEBUGLOG("test_tcpclient_tinypbproto : write message success.");
        });

        client.readMessage("001", [](rayrpc::AbstractProtocol::s_ptr protomsg) {
            std::shared_ptr<rayrpc::TinyPBProtocol> tinypbmsg = std::dynamic_pointer_cast<rayrpc::TinyPBProtocol>(protomsg);
            DEBUGLOG("test_tcpclient_tinypbproto : read [%s] message success, [%s]", tinypbmsg->getReqId().c_str(), tinypbmsg->m_pb_data.c_str());
        });
    });
}


int main() {
    rayrpc::Config::setGlobalConfig("../../rayrpc.xml");

    rayrpc::Logger::initGlobalLogger();

    // test_connect();

    test_tcpclient_tinypbproto();

    return 0;
}