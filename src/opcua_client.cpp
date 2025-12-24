#include "../include/opcua_client.hpp"
#include <chrono>
#include <ctime>
#include <cstdio>

OPCUAClient::OPCUAClient() : connected(false) {
    client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));
    tags.emplace_back("Temperature", "ns=2;i=1");
    tags.emplace_back("Voltage", "ns=2;i=2");
}

OPCUAClient::~OPCUAClient() {
    if (connected) UA_Client_disconnect(client);
    UA_Client_delete(client);
}

bool OPCUAClient::connectToServer(const std::string& url) {
    UA_StatusCode retval = UA_Client_connect(client, url.c_str());
    connected = (retval == UA_STATUSCODE_GOOD);
    return connected;
}

void OPCUAClient::updateValues() {
    if (!connected) return;
    std::lock_guard<std::mutex> lock(tags_mutex);

    for (auto& tag : tags) {
        UA_Variant val;
        UA_Variant_init(&val);
        int ns, id;
        if(sscanf(tag.nodeId.c_str(), "ns=%d;i=%d", &ns, &id) == 2) {
            UA_NodeId nid = UA_NODEID_NUMERIC((UA_UInt16)ns, (UA_UInt32)id);
            if (UA_Client_readValueAttribute(client, nid, &val) == UA_STATUSCODE_GOOD) {
                if (val.type == &UA_TYPES[UA_TYPES_DOUBLE]) tag.value = *(UA_Double*)val.data;
                else if (val.type == &UA_TYPES[UA_TYPES_FLOAT]) tag.value = (double)*(UA_Float*)val.data;
                
                auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                char buf[12];
                std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
                tag.timestamp = buf;
                tag.quality = "GOOD";
            }
        }
        UA_Variant_clear(&val);
    }
}

bool OPCUAClient::writeValue(const std::string& nodeId, double newValue) {
    if (!connected) return false;
    UA_Variant val;
    UA_Variant_init(&val);
    UA_Variant_setScalarCopy(&val, &newValue, &UA_TYPES[UA_TYPES_DOUBLE]);
    int ns, id;
    sscanf(nodeId.c_str(), "ns=%d;i=%d", &ns, &id);
    UA_NodeId nid = UA_NODEID_NUMERIC((UA_UInt16)ns, (UA_UInt32)id);
    UA_StatusCode res = UA_Client_writeValueAttribute(client, nid, &val);
    UA_Variant_clear(&val);
    return (res == UA_STATUSCODE_GOOD);
}

std::vector<OPCUAClient::TagData> OPCUAClient::getTags() {
    std::lock_guard<std::mutex> lock(tags_mutex);
    return tags;
}