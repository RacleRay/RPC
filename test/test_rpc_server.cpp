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


// From test.pb.h
class CommImpl : public Comm {
  public:
    void testComm(
        google::protobuf::RpcController *controller,
        const ::testCommRequest *request,
        ::testCommResponse *response,
        google::protobuf::Closure *done) override 
    {
        DEBUGLOG("start sleep 5s");
        sleep(5);
        DEBUGLOG("end sleep 5s");
        
        if (request->count() < 10) {
            response->set_ret_code(-1);
            response->set_res_info("test limit count with 10");
            return;
        }
        response->set_more_info("basic test");
    }
};


void test_tcp_server() {
    rayrpc::IPNetAddr::s_ptr addr = std::make_shared<rayrpc::IPNetAddr>("127.0.0.1", 12345);

    DEBUGLOG("test tcp server : local addr %s", addr->toString().c_str());

    rayrpc::TcpServer tcp_server(addr);

    tcp_server.start();
}


int main() {
    rayrpc::Config::setGlobalConfig("../../rayrpc.xml");
    rayrpc::Logger::initGlobalLogger();

    std::shared_ptr<CommImpl> service = std::make_shared<CommImpl>();
    rayrpc::RpcDispatcher::GetRpcDispatcher()->registerService(service);

    test_tcp_server();

    return 0;
}
