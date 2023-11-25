#pragma once

#include <map>
#include <string>

namespace rayrpc {

class Config {
  public:
    explicit Config(const char *xmlfile);

  public:
    static Config *getGlobalConfig();
    static void setGlobalConfig(const char *xmlfile);

  public:
    std::string m_log_level;
};

} // namespace rayrpc