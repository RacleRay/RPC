#include "config.h"
#include "log.h"
#include "tcp/netaddr.h"


int main() {
  rayrpc::Config::setGlobalConfig("../../rayrpc.xml");
  rayrpc::Logger::initGlobalLogger();

  rayrpc::IPNetAddr addr("127.0.0.1", 12345);
  DEBUGLOG("create addr %s", addr.toString().c_str());
  
  return 0;
}