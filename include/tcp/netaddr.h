#pragma once

#include <arpa/inet.h>
#include <memory>
#include <netinet/in.h>
#include <string>


namespace rayrpc {

class NetAddr {
  public:
    using s_ptr = std::shared_ptr<NetAddr>;

    virtual sockaddr *getSockAddr() = 0;

    virtual socklen_t getSockAddrLen() = 0;

    virtual int getFamily() = 0;

    virtual std::string toString() = 0;

    virtual bool checkValid() = 0;
}; // class NetAddr


class IPNetAddr : public NetAddr {
  public:
    IPNetAddr(std::string ip, uint16_t port);
    explicit IPNetAddr(const std::string &addr);
    explicit IPNetAddr(sockaddr_in addr);

    ~IPNetAddr() = default;

    sockaddr *getSockAddr() override;

    socklen_t getSockAddrLen() override;

    int getFamily() override;

    std::string toString() override;

    bool checkValid() override;

  private:
    std::string m_ip;
    uint16_t m_port{0};
    sockaddr_in m_addr;
}; // class IPNetAddr

} // namespace rayrpc