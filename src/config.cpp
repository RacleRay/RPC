#include <tinyxml/tinyxml.h>

#include "config.h"

namespace {
rayrpc::Config *g_config = nullptr;

#define READ_XML_NODE(name, parent)                                          \
    TiXmlElement *name##_node = (parent)->FirstChildElement(#name);          \
    if (!name##_node) {                                                      \
        printf("Start rpc server error, failed to read node [%s]\n", #name); \
        exit(0);                                                             \
    }

#define READ_STR_FROM_XML_NODE(name, parent)                                   \
    TiXmlElement *name##_node = (parent)->FirstChildElement(#name);            \
    if (!name##_node || !name##_node->GetText()) {                             \
        printf(                                                                \
            "Start rpc server error, failed to read config file %s\n", #name); \
        exit(0);                                                               \
    }                                                                          \
    std::string name##_str = std::string(name##_node->GetText());
} // namespace


namespace rayrpc {

Config *Config::getGlobalConfig() {
    return g_config;
}

void Config::setGlobalConfig(const char *xmlfile, ConfigType type) {
    if (g_config == nullptr) { g_config = new Config(xmlfile, type); }
}

Config::Config(const char *xmlfile, ConfigType type) {
    m_config_type = type;
    // auto *doc = new TiXmlDocument();

    m_xml_document = new TiXmlDocument();
    if (!m_xml_document->LoadFile(xmlfile)) {
        printf("Start rpc server error, failed to read config file %s, error info[%s] \n",
                xmlfile, m_xml_document->ErrorDesc());
        exit(0);
    }

    READ_XML_NODE(root, m_xml_document);
    READ_XML_NODE(log, root_node);
    READ_XML_NODE(server, root_node);

    READ_STR_FROM_XML_NODE(log_level, log_node);
    m_log_level = log_level_str;

    READ_STR_FROM_XML_NODE(ip, server_node);
    READ_STR_FROM_XML_NODE(port, server_node);
    m_server_ip = ip_str;
    m_port = (int)std::strtol(port_str.c_str(), nullptr, 10);

    if (type == ConfigType::ClientConfig) {
        printf("CLIENT -- CONFIG LEVEL[%s], SERVER_IP[%s], SERVER_PORT[%d]\n", 
            m_log_level.c_str(), m_server_ip.c_str(), m_port);
        return;
    }

    READ_STR_FROM_XML_NODE(log_file_name, log_node);
    READ_STR_FROM_XML_NODE(log_file_path, log_node);
    READ_STR_FROM_XML_NODE(log_max_file_size, log_node);
    READ_STR_FROM_XML_NODE(log_sync_interval, log_node);

    m_log_file_name = log_file_name_str;
    m_log_file_path = log_file_path_str;
    m_log_max_file_size = (int)std::strtol(log_max_file_size_str.c_str(), nullptr, 10);
    m_log_sync_inteval = (int)std::strtol(log_sync_interval_str.c_str(), nullptr, 10);
    printf("LOG -- CONFIG LEVEL[%s], FILE_NAME[%s], FILE_PATH[%s], MAX_FILE_SIZE[%d B], SYNC_INTEVAL[%d ms]\n", 
            m_log_level.c_str(), 
            m_log_file_name.c_str(), 
            m_log_file_path.c_str(), 
            m_log_max_file_size, 
            m_log_sync_inteval);

    READ_STR_FROM_XML_NODE(io_threads, server_node);
    m_io_threads = (int)std::strtol(io_threads_str.c_str(), nullptr, 10);

    // 保存 服务名 到 stub 桩的映射，从服务名可以找到对应的服务器地址
    TiXmlElement* stubs_node = root_node->FirstChildElement("stubs");
    if (stubs_node) {
        for (TiXmlElement* node = stubs_node->FirstChildElement("rpc_server"); node; node = node->NextSiblingElement("rpc_server")) {
            RpcStub stub;
            stub.name = std::string(node->FirstChildElement("name")->GetText());
            stub.timeout = (int)std::strtol(node->FirstChildElement("timeout")->GetText(), nullptr, 10);
            
            std::string ip = std::string(node->FirstChildElement("ip")->GetText());
            auto port = (uint16_t)std::strtol(node->FirstChildElement("port")->GetText(), nullptr, 10);
            stub.addr = std::make_shared<IPNetAddr>(ip, port);

            m_rpc_stubs.insert(std::make_pair(stub.name, stub));
        }
    }

    printf("SERVER -- CONFIG PORT[%d], IO_THREADS[%d]\n", m_port, m_io_threads);
}


Config::~Config() {
    if (m_xml_document) {
        delete m_xml_document;
        m_xml_document = nullptr;
    }
}


} // namespace rayrpc