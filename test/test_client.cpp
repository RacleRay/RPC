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

#include "config.h"
#include "log.h"
#include "protocol/abstractproto.h"
#include "protocol/stringcoder.h"
#include "tcp/netaddr.h"
#include "tcp/tcpclient.h"

// echo 测试 
// 应该返回  hello rayrpc! 
void test_connect() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        ERRLOG("invalid fd %d", fd);
        exit(0);
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_aton("127.0.0.1", &server_addr.sin_addr);

    int ret = connect(fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr));
    DEBUGLOG("test_connect : connect success");

    std::string msg = "hello rayrpc!";

    ret = write(fd, msg.c_str(), msg.length());
    DEBUGLOG("test_connect : success write %d bytes, [%s]", ret, msg.c_str());

    char buf[100];
    ret = read(fd, buf, 100);
    DEBUGLOG("test_connect : success read %d bytes, [%s]", ret, std::string(buf).c_str());
}


// !!! 运行需要在 TcpConnection::TcpConnection 中，修改 m_coder 为 new StringCoder().
void test_tcpclient_stringproto() {
    rayrpc::IPNetAddr::s_ptr addr = std::make_shared<rayrpc::IPNetAddr>("127.0.0.1", 12345);
    rayrpc::TcpClient client(addr);

    client.connect([addr, &client]() {
        DEBUGLOG("test_tcpclient_stringproto : connect to [%s] success.", addr->toString().c_str());

        std::shared_ptr<rayrpc::StringProtocol> proto = std::make_shared<rayrpc::StringProtocol>();
        proto->info = "test : hello rayrpc!";
        proto->setReqId("001");

        client.writeMessage(proto, [](rayrpc::AbstractProtocol::s_ptr proto) {
            DEBUGLOG("test_tcpclient_stringproto : write message success.");
        });

        client.readMessage("001", [](rayrpc::AbstractProtocol::s_ptr proto) {
            std::shared_ptr<rayrpc::StringProtocol> strproto = std::dynamic_pointer_cast<rayrpc::StringProtocol>(proto);
            DEBUGLOG("test_tcpclient_stringproto : read [%s] message success, [%s]", strproto->getReqId().c_str(), strproto->info.c_str());
        });

        client.writeMessage(proto, [](rayrpc::AbstractProtocol::s_ptr proto) {
            DEBUGLOG("test_tcpclient_stringproto : write second message success.");
        });
    });
}


int main() {
    rayrpc::Config::setGlobalConfig("../../rayrpc.xml");

    rayrpc::Logger::initGlobalLogger();

    // test_connect();

    test_tcpclient_stringproto();

    return 0;
}