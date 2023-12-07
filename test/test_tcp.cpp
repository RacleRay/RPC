#include "config.h"
#include "log.h"
#include "tcp/netaddr.h"
#include "tcp/tcpserver.h"


void test_tcp_server() {
  rayrpc::IPNetAddr::s_ptr addr = std::make_shared<rayrpc::IPNetAddr>("127.0.0.1", 12345);

  DEBUGLOG("test_tcp_server : create addr %s", addr->toString().c_str());

  rayrpc::TcpServer tcp_server(addr);

  tcp_server.start();

}


int main() {
  rayrpc::Config::setGlobalConfig("../../rayrpc.xml");
  rayrpc::Logger::initGlobalLogger();

  test_tcp_server();
  
  return 0;
}