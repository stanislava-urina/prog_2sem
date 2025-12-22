#include "../include/opcua_client.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>

#if USE_REAL_OPCUA
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#endif

using namespace std;

// ==================== КОНСТРУКТОР / ДЕСТРУКТОР ====================

OPCUAClient::OPCUAClient() {
#if USE_REAL_OPCUA
    client = UA_Client_new();
    UA_ClientConfig* config = UA_Client_getConfig(client);
    UA_ClientConfig_setDefault(config);
    config->timeout = 3000;
#endif
}

OPCUAClient::~OPCUAClient() {
    disconnect();
#if USE_REAL_OPCUA
    if (client) {
        UA_Client_delete(client);
    }
#endif
}

// ==================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ====================

std::string OPCUAClient::nodeIdToString(const UA_NodeId* nodeId) {
    if (!nodeId || UA_NodeId_isNull(nodeId)) return "";
    
#if USE_REAL_OPCUA
    UA_String str = UA_STRING_NULL;
    UA_NodeId_print(nodeId, &str);
    
    std::string result;
    if (str.length > 0) {
        result = std::string((char*)str.data, str.length);
    }
    
    UA_String_clear(&str);
    return result;
#else
    return "ns=0;i=0";
#endif
}

std::string OPCUAClient::getNodeDisplayName(const UA_NodeId* nodeId) {
#if USE_REAL_OPCUA
    if (!client || !nodeId) return "";
    
    UA_LocalizedText displayName = UA_LOCALIZEDTEXT("", "");
    UA_StatusCode retval = UA_Client_readDisplayNameAttribute(client, *nodeId, &displayName);
    
    if (retval == UA_STATUSCODE_GOOD && displayName.text.length > 0) {
        std::string name((char*)displayName.text.data, displayName.text.length);
        UA_LocalizedText_clear(&displayName);
        return name;
    }
    
    UA_LocalizedText_clear(&displayName);
#endif
    return nodeIdToString(nodeId);
}

std::string OPCUAClient::getEngineeringUnits(const UA_NodeId* nodeId) {
#if USE_REAL_OPCUA
    if (!client || !nodeId) return "";
    
    // Пытаемся прочитать EngineeringUnits
    UA_Variant value;
    UA_Variant_init(&value);
    UA_StatusCode retval = UA_Client_readValueAttribute(client, *nodeId, &value);
    
    if (retval == UA_STATUSCODE_GOOD) {
        // Проверяем, есть ли EUInformation в расширенных данных
        // В реальном проекте здесь был бы более сложный код
        UA_Variant_clear(&value);
        return "Units";
    }
    
    UA_Variant_clear(&value);
#endif
    return "";
}

void OPCUAClient::browseNodeInternal(const UA_NodeId* nodeId, int depth) {
    if (depth > 3) return;
    
#if USE_REAL_OPCUA
    if (!client || !nodeId) return;
    
    UA_BrowseRequest bReq;
    UA_BrowseRequest_init(&bReq);
    bReq.requestedMaxReferencesPerNode = 0;
    bReq.nodesToBrowse = UA_BrowseDescription_new();
    bReq.nodesToBrowseSize = 1;
    
    UA_BrowseDescription_init(&bReq.nodesToBrowse[0]);
    bReq.nodesToBrowse[0].nodeId = *nodeId;
    bReq.nodesToBrowse[0].resultMask = UA_BROWSERESULTMASK_ALL;
    
    UA_BrowseResponse bResp = UA_Client_Service_browse(client, bReq);
    
    if (bResp.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
        for (size_t i = 0; i < bResp.resultsSize; ++i) {
            UA_BrowseResult* result = &bResp.results[i];
            if (result->statusCode == UA_STATUSCODE_GOOD && result->referencesSize > 0) {
                for (size_t j = 0; j < result->referencesSize; ++j) {
                    UA_ReferenceDescription* ref = &result->references[j];
                    
                    if (ref->nodeClass == UA_NODECLASS_VARIABLE) {
                        std::string nodeIdStr = nodeIdToString(&ref->nodeId.nodeId);
                        std::string name = getNodeDisplayName(&ref->nodeId.nodeId);
                        std::string unit = getEngineeringUnits(&ref->nodeId.nodeId);
                        
                        // Читаем значение для проверки типа
                        UA_Variant val;
                        UA_Variant_init(&val);
                        UA_StatusCode readStatus = UA_Client_readValueAttribute(client, 
                                                                               ref->nodeId.nodeId, 
                                                                               &val);
                        
                        if (readStatus == UA_STATUSCODE_GOOD) {
                            // Проверяем числовой тип
                            bool isNumeric = false;
                            if (UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_DOUBLE]) ||
                                UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_FLOAT]) ||
                                UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_INT32]) ||
                                UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_UINT32]) ||
                                UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_INT16]) ||
                                UA_Variant_hasScalarType(&val, &UA_TYPES[UA_TYPES_UINT16])) {
                                isNumeric = true;
                            }
                            
                            if (isNumeric && !name.empty()) {
                                addTag(name, nodeIdStr, unit, 0, 100);
                                cout << "[OPC UA] Found tag: " << name 
                                     << " [" << nodeIdStr << "]" << endl;
                            }
                        }
                        
                        UA_Variant_clear(&val);
                    }
                    
                    // Рекурсивный обход
                    if ((ref->nodeClass == UA_NODECLASS_OBJECT || 
                         ref->nodeClass == UA_NODECLASS_VARIABLETYPE) && depth < 3) {
                        browseNodeInternal(&ref->nodeId.nodeId, depth + 1);
                    }
                }
            }
        }
    }
    
    UA_BrowseRequest_clear(&bReq);
    UA_BrowseResponse_clear(&bResp);
#endif
}

// ==================== ОСНОВНЫЕ ФУНКЦИИ ====================

bool OPCUAClient::connect(const std::string& url) {
    endpoint = url;
    cout << "[OPC UA] Connecting to " << url << "..." << endl;
    
#if USE_REAL_OPCUA
    UA_StatusCode retval = UA_Client_connect(client, url.c_str());
    if (retval == UA_STATUSCODE_GOOD) {
        connected = true;
        cout << "[OPC UA] ✅ Connected successfully!" << endl;
        return true;
    } else {
        cout << "[OPC UA] ❌ Connection failed: " << UA_StatusCode_name(retval) << endl;
        connected = false;
        return false;
    }
#else
    cout << "[OPC UA] Simulation mode - not really connecting" << endl;
    connected = true;
    return true;
#endif
}

void OPCUAClient::disconnect() {
#if USE_REAL_OPCUA
    if (client && connected) {
        UA_Client_disconnect(client);
        cout << "[OPC UA] Disconnected" << endl;
    }
#endif
    connected = false;
}

bool OPCUAClient::isConnected() const {
    return connected;
}

bool OPCUAClient::discoverTags() {
    cout << "[OPC UA] Discovering tags..." << endl;
    
    {
        lock_guard<mutex> lock(tags_mutex);
        tags.clear();
        tagHistories.clear();
    }
    
#if USE_REAL_OPCUA
    if (!client || !connected) {
        cout << "[OPC UA] Not connected!" << endl;
        return false;
    }
    
    // Начинаем обход с ObjectsFolder
    UA_NodeId objectsFolder = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    browseNodeInternal(&objectsFolder, 0);
    
    if (tags.empty()) {
        cout << "[OPC UA] No tags found. Adding demo tags..." << endl;
        addTag("Temperature", "ns=2;i=1", "°C", 20, 30);
        addTag("Pressure", "ns=2;i=2", "bar", 1, 2);
        addTag("Voltage", "ns=2;i=3", "V", 220, 240);
    }
#else
    // Демо-теги для симуляции
    addTag("Temperature_Room1", "ns=2;i=1", "°C", 20.0, 30.0);
    addTag("Pressure_TankA", "ns=2;i=2", "bar", 1.0, 2.0);
    addTag("Voltage_Phase_A", "ns=2;i=3", "V", 220.0, 240.0);
    addTag("FlowRate_Pump1", "ns=2;i=4", "m³/h", 40.0, 60.0);
    addTag("Level_Tank2", "ns=2;i=5", "%", 70.0, 80.0);
#endif
    
    cout << "[OPC UA] Total tags: " << tagCount() << endl;
    return tagCount() > 0;
}

void OPCUAClient::updateValues() {
    lock_guard<mutex> lock(tags_mutex);
    
    for (auto& tag : tags) {
        if (tag.is_written) continue;
        
        double newValue = 0.0;
        bool success = false;
        
#if USE_REAL_OPCUA
        if (client && connected) {
            // Парсим NodeId из строки
            UA_NodeId nodeId = UA_NODEID_NULL;
            
            // Простой парсинг для формата "ns=X;i=Y"
            int ns, id;
            if (sscanf_s(tag.nodeId.c_str(), "ns=%d;i=%d", &ns, &id) == 2) {
                nodeId = UA_NODEID_NUMERIC(ns, id);
            } 
            // Для строковых идентификаторов "ns=X;s=STRING"
            else if (tag.nodeId.find("ns=") != string::npos && 
                     tag.nodeId.find("s=") != string::npos) {
                size_t nsPos = tag.nodeId.find("ns=");
                size_t sPos = tag.nodeId.find("s=");
                if (nsPos != string::npos && sPos != string::npos) {
                    int ns = stoi(tag.nodeId.substr(nsPos + 3, sPos - nsPos - 4));
                    string strId = tag.nodeId.substr(sPos + 2);
                    nodeId = UA_NODEID_STRING_ALLOC(ns, strId.c_str());
                }
            }
            
            if (!UA_NodeId_isNull(&nodeId)) {
                UA_Variant value;
                UA_Variant_init(&value);
                UA_StatusCode retval = UA_Client_readValueAttribute(client, nodeId, &value);
                
                if (retval == UA_STATUSCODE_GOOD) {
                    if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) {
                        newValue = *(UA_Double*)value.data;
                        success = true;
                    }
                    else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_FLOAT])) {
                        newValue = *(UA_Float*)value.data;
                        success = true;
                    }
                    else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
                        newValue = *(UA_Int32*)value.data;
                        success = true;
                    }
                    else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT32])) {
                        newValue = *(UA_UInt32*)value.data;
                        success = true;
                    }
                    else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT16])) {
                        newValue = *(UA_Int16*)value.data;
                        success = true;
                    }
                    else if (UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_UINT16])) {
                        newValue = *(UA_UInt16*)value.data;
                        success = true;
                    }
                }
                
                UA_Variant_clear(&value);
                if (nodeId.identifierType == UA_NODEIDTYPE_STRING) {
                    UA_NodeId_clear(&nodeId);
                }
            }
        }
#else
        // Симуляция для тестирования GUI
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        if (tag.name.find("Temperature") != string::npos) {
            std::uniform_real_distribution<> dist(20.0, 30.0);
            newValue = dist(gen);
        } else if (tag.name.find("Pressure") != string::npos) {
            std::uniform_real_distribution<> dist(1.0, 2.0);
            newValue = dist(gen);
        } else if (tag.name.find("Voltage") != string::npos) {
            std::uniform_real_distribution<> dist(220.0, 240.0);
            newValue = dist(gen);
        } else {
            std::uniform_real_distribution<> dist(50.0, 100.0);
            newValue = dist(gen);
        }
        success = true;
#endif
        
        if (success) {
            addToHistory(tag.name, tag.value, tag.timestamp);
            tag.update(newValue, false);
            tag.quality = "GOOD";
        } else {
            tag.quality = "BAD";
        }
    }
}

bool OPCUAClient::writeTagByName(const std::string& tagName, double value) {
    lock_guard<mutex> lock(tags_mutex);
    
    for (auto& tag : tags) {
        if (tag.name == tagName) {
            addToHistory(tagName, tag.value, tag.timestamp);
            
#if USE_REAL_OPCUA
            if (client && connected) {
                UA_NodeId nodeId = UA_NODEID_NULL;
                int ns, id;
                
                if (sscanf_s(tag.nodeId.c_str(), "ns=%d;i=%d", &ns, &id) == 2) {
                    nodeId = UA_NODEID_NUMERIC(ns, id);
                }
                
                if (!UA_NodeId_isNull(&nodeId)) {
                    UA_Variant var;
                    UA_Variant_init(&var);
                    UA_Double dvalue = value;
                    UA_Variant_setScalar(&var, &dvalue, &UA_TYPES[UA_TYPES_DOUBLE]);
                    
                    UA_StatusCode retval = UA_Client_writeValueAttribute(client, nodeId, &var);
                    UA_Variant_clear(&var);
                    
                    if (retval == UA_STATUSCODE_GOOD) {
                        tag.update(value, true);
                        tag.quality = "GOOD";
                        cout << "[OPC UA] ✓ Written to server: " << tagName << " = " << value << endl;
                        return true;
                    } else {
                        cout << "[OPC UA] ✗ Write failed: " << UA_StatusCode_name(retval) << endl;
                        return false;
                    }
                }
            }
#endif
            // Если не реальный OPC UA или не удалось записать - всё равно обновляем локально
            tag.update(value, true);
            tag.quality = "GOOD";
            cout << "[OPC UA] Tag updated locally: " << tagName << " = " << value << endl;
            return true;
        }
    }
    
    cout << "[OPC UA] Tag not found: " << tagName << endl;
    return false;
}

bool OPCUAClient::writeTagById(const std::string& nodeId, double value) {
    lock_guard<mutex> lock(tags_mutex);
    
    for (auto& tag : tags) {
        if (tag.nodeId == nodeId) {
            return writeTagByName(tag.name, value);
        }
    }
    
    cout << "[OPC UA] NodeId not found: " << nodeId << endl;
    return false;
}

// ==================== ГЕТТЕРЫ И СЕТТЕРЫ ====================

std::vector<OPCUAClient::TagData> OPCUAClient::getTags() const {
    lock_guard<mutex> lock(tags_mutex);
    return tags;
}

size_t OPCUAClient::tagCount() const {
    lock_guard<mutex> lock(tags_mutex);
    return tags.size();
}

OPCUAClient::TagData* OPCUAClient::getTagByName(const std::string& tagName) {
    lock_guard<mutex> lock(tags_mutex);
    for (auto& tag : tags) {
        if (tag.name == tagName) {
            return &tag;
        }
    }
    return nullptr;
}

bool OPCUAClient::resetTagToAuto(const std::string& tagName) {
    lock_guard<mutex> lock(tags_mutex);
    
    for (auto& tag : tags) {
        if (tag.name == tagName && tag.is_written) {
            tag.is_written = false;
            cout << "[OPC UA] Tag reset to AUTO: " << tagName << endl;
            return true;
        }
    }
    
    cout << "[OPC UA] Tag not found or not WRITTEN: " << tagName << endl;
    return false;
}

void OPCUAClient::resetAllWrittenTags() {
    lock_guard<mutex> lock(tags_mutex);
    
    int count = 0;
    for (auto& tag : tags) {
        if (tag.is_written) {
            tag.is_written = false;
            count++;
        }
    }
    
    cout << "[OPC UA] Reset " << count << " tags to AUTO mode" << endl;
}

void OPCUAClient::addTag(const std::string& name, const std::string& nodeId, 
                        const std::string& unit, double minVal, double maxVal) {
    lock_guard<mutex> lock(tags_mutex);
    TagData tag(name, nodeId, unit);
    tag.value = (minVal + maxVal) / 2.0;
    tag.updateTimestamp();
    tags.push_back(tag);
    cout << "[OPC UA] Added tag: " << name << " [" << nodeId << "]" << endl;
}

// ==================== ИСТОРИЯ ====================

OPCUAClient::TagHistory* OPCUAClient::getTagHistory(const std::string& tagName) {
    lock_guard<mutex> lock(history_mutex);
    auto it = tagHistories.find(tagName);
    if (it != tagHistories.end()) {
        return &it->second;
    }
    return nullptr;
}

void OPCUAClient::addToHistory(const std::string& tagName, double value, const std::string& timestamp) {
    lock_guard<mutex> lock(history_mutex);
    tagHistories[tagName].addValue(value, timestamp);
}

// ==================== МЕТОДЫ TAGDATA ====================

void OPCUAClient::TagData::update(double newValue, bool written) {
    value = newValue;
    is_written = written;
    updateTimestamp();
}

void OPCUAClient::TagData::updateTimestamp() {
    auto now = chrono::system_clock::now();
    auto time = chrono::system_clock::to_time_t(now);
    tm tm;
    localtime_s(&tm, &time);
    char timeStr[100];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tm);
    timestamp = timeStr;
}

void OPCUAClient::TagData::print() const {
    cout << left 
          << setw(25) << name 
          << setw(12) << fixed << setprecision(3) << value 
          << setw(8) << unit 
          << setw(10) << timestamp 
          << setw(10) << quality 
          << (is_written ? " [WRITTEN]" : " [AUTO]")
          << endl;
}

// ==================== МЕТОДЫ TAGHISTORY ====================

void OPCUAClient::TagHistory::addValue(double value, const std::string& timestamp) {
    values.push_back(value);
    timestamps.push_back(timestamp);
    
    if (values.size() > maxHistory) {
        values.erase(values.begin());
        timestamps.erase(timestamps.begin());
    }
}

void OPCUAClient::TagHistory::clear() {
    values.clear();
    timestamps.clear();
}