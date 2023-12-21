#pragma once

#include <map>
#include <string>
#include <tinyxml/tinyxml.h>

#include "log.h"

namespace rayrpc {

enum class ConfigType { ServerConfig = 1, ClientConfig = 2 };

class Config {
  public:
    explicit Config(const char *xmlfile, ConfigType type = ConfigType::ServerConfig);

    ~Config();

  public:
    static Config *getGlobalConfig();
    static void setGlobalConfig(const char *xmlfile, ConfigType type = ConfigType::ServerConfig);

  public:
    ConfigType m_config_type;

    TiXmlDocument* m_xml_document;

    std::string m_log_level;
    std::string m_log_file_name;
    std::string m_log_file_path;
    int m_log_max_file_size {0};  // 日志文件最大大小，B
    int m_log_sync_inteval {0};   // 日志同步间隔，ms
    
    std::string m_server_ip;
    int m_port {0};
    int m_io_threads {0};
};

} // namespace rayrpc