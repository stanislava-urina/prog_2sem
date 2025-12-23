#include "../include/opcua_client.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <ctime>
#include <map>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if USE_REAL_OPCUA
#include <open62541/client_highlevel.h>
#include <open62541/client_config_default.h>
#endif

// ==================== КОНСТРУКТОР / ДЕСТРУКТОР ====================

OPCUAClient::OPCUAClient() : connected(false) {
#if USE_REAL_OPCUA
    client = UA_Client_new();
    UA_ClientConfig* config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(config);
    config->timeout = 5000;
#endif
    // Добавляем теги сразу при создании клиента
    addTag("Temperature", "ns=2;i=1", "C", 0, 100);
    addTag("Voltage", "ns=2;i=2", "V", 0, 500);
}

OPCUAClient::~OPCUAClient() {
    disconnect();
#if USE_REAL_OPCUA
    if (client) UA_Client_delete(client);
#endif
}

// ==================== СВЯЗЬ ====================

bool OPCUAClient::connect(const std::string& url) {
    endpoint = url;
#if USE_REAL_OPCUA
    if (!client) return false;
    UA_StatusCode retval = UA_Client_connect(client, url.c_str());
    if (retval != UA_STATUSCODE_GOOD) {
        connected = false;
        return false;
    }
    connected = true;
    return true;
#else
    connected = true;
    return true;
#endif
}

void OPCUAClient::disconnect() {
#if USE_REAL_OPCUA
    if (connected && client) {
        UA_Client_disconnect(client);
    }
#endif
    connected = false;
}

bool OPCUAClient::discoverTags() {
    return !tags.empty();
}

// ==================== ЧТЕНИЕ И ЗАПИСЬ ====================

void OPCUAClient::updateValues() {
    std::lock_guard<std::mutex> lock(tags_mutex);
    if (!connected) return;

#if USE_REAL_OPCUA
    for (auto& tag : tags) {
        if (tag.is_written) continue; 

        UA_Variant val;
        UA_Variant_init(&val);
        
        int ns = 0, id = 0;
        if (sscanf_s(tag.nodeId.c_str(), "ns=%d;i=%d", &ns, &id) == 2) {
            UA_NodeId nid = UA_NODEID_NUMERIC(ns, id);
            UA_StatusCode retval = UA_Client_readValueAttribute(client, nid, &val);

            if (retval == UA_STATUSCODE_GOOD) {
                double nv = 0;
                if (val.type == &UA_TYPES[UA_TYPES_DOUBLE]) nv = *(UA_Double*)val.data;
                else if (val.type == &UA_TYPES[UA_TYPES_FLOAT]) nv = (double)*(UA_Float*)val.data;
                
                tag.update(nv, false);
                tag.quality = "GOOD";
                
                // Сохраняем историю (вызов метода внутри класса)
                addToHistory(tag.name, tag.value, tag.timestamp);
            } else {
                tag.quality = "BAD";
            }
        }
        UA_Variant_clear(&val);
    }
#endif
}

bool OPCUAClient::writeTagByName(const std::string& name, double val) {
    std::lock_guard<std::mutex> lock(tags_mutex);
    
    TagData* tag = nullptr;
    for (auto& t : tags) { if (t.name == name) { tag = &t; break; } }
    
    if (!tag || !connected) return false;

#if USE_REAL_OPCUA
    UA_Variant myValue;
    UA_Variant_init(&myValue);
    UA_Variant_setScalarCopy(&myValue, &val, &UA_TYPES[UA_TYPES_DOUBLE]);

    int ns = 0, id = 0;
    if (sscanf_s(tag->nodeId.c_str(), "ns=%d;i=%d", &ns, &id) == 2) {
        UA_NodeId nid = UA_NODEID_NUMERIC(ns, id);
        UA_StatusCode retval = UA_Client_writeValueAttribute(client, nid, &myValue);
        UA_Variant_clear(&myValue);

        if (retval == UA_STATUSCODE_GOOD) {
            tag->update(val, true);
            return true;
        }
    }
    UA_Variant_clear(&myValue);
#endif
    return false;
}

// ==================== ГЕТТЕРЫ И СЕРВИСНЫЕ МЕТОДЫ ====================

std::vector<OPCUAClient::TagData> OPCUAClient::getTags() const { 
    std::lock_guard<std::mutex> lock(tags_mutex);
    return tags; 
}

size_t OPCUAClient::tagCount() const { 
    std::lock_guard<std::mutex> lock(tags_mutex);
    return tags.size(); 
}
OPCUAClient::TagData* OPCUAClient::getTagByName(const std::string& name) {
    // Используем обычный цикл без lock_guard здесь для простоты, 
    // так как вызывается обычно из потока GUI
    for (size_t i = 0; i < tags.size(); ++i) {
        if (tags[i].name == name) return &tags[i];
    }
    return nullptr;
}

OPCUAClient::TagHistory* OPCUAClient::getTagHistory(const std::string& name) {
    std::lock_guard<std::mutex> lock(history_mutex);
    if (tagHistories.find(name) != tagHistories.end()) return &tagHistories[name];
    return nullptr;
}

void OPCUAClient::addToHistory(const std::string& name, double val, const std::string& ts) {
    std::lock_guard<std::mutex> lock(history_mutex);
    tagHistories[name].addValue(val, ts);
}

void OPCUAClient::addTag(const std::string& n, const std::string& id, const std::string& u, double min, double max) {
    TagData t(n, id, u);
    t.quality = "INIT";
    tags.push_back(t);
}

bool OPCUAClient::isConnected() const { return connected; }

bool OPCUAClient::writeTagById(const std::string& i, double v) { 
    return false; 
}

bool OPCUAClient::resetTagToAuto(const std::string& n) { 
    std::lock_guard<std::mutex> lock(tags_mutex);
    for(auto& t : tags) if(t.name == n) { t.is_written = false; return true; }
    return false; 
}

void OPCUAClient::resetAllWrittenTags() {
    std::lock_guard<std::mutex> lock(tags_mutex);
    for(auto& t : tags) t.is_written = false;
}

std::string OPCUAClient::nodeIdToString(const UA_NodeId* n) { return ""; }
std::string OPCUAClient::getNodeDisplayName(const UA_NodeId* n) { return ""; }
std::string OPCUAClient::getEngineeringUnits(const UA_NodeId* n) { return ""; }
void OPCUAClient::browseNodeInternal(const UA_NodeId* n, int d) {}

// ==================== МЕТОДЫ СТРУКТУР ====================

void OPCUAClient::TagData::update(double nv, bool w) {
    value = nv;
    is_written = w;
    updateTimestamp();
}

void OPCUAClient::TagData::updateTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm lt;
    localtime_s(&lt, &t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &lt);
    timestamp = buf;
}

void OPCUAClient::TagHistory::addValue(double v, const std::string& t) {
    values.push_back(v);
    timestamps.push_back(t);
    if (values.size() > maxHistory) {
        values.erase(values.begin());
        timestamps.erase(timestamps.begin());
    }
}

void OPCUAClient::TagData::print() const {}