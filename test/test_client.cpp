#include "config.h"
#include "log.h"
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

int main() {
    rayrpc::Config::setGlobalConfig("../../rayrpc.xml");

    rayrpc::Logger::initGlobalLogger();

    test_connect();

    return 0;
}