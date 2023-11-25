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

void Config::setGlobalConfig(const char *xmlfile) {
    if (g_config == nullptr) { g_config = new Config(xmlfile); }
}

Config::Config(const char *xmlfile) {
    auto *doc = new TiXmlDocument();
    if (!doc->LoadFile(xmlfile)) {
        printf(
            "Start rpc server error, failed to read config file %s, error info[%s] \n",
            xmlfile, doc->ErrorDesc());
        exit(0);
    }

    READ_XML_NODE(root, doc);
    READ_XML_NODE(log, root_node);

    READ_STR_FROM_XML_NODE(log_level, log_node);

    m_log_level = log_level_str;
}

} // namespace rayrpc