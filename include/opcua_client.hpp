#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include <ctime>
#include <mutex>
#include <map>

// ВКЛЮЧАЕМ РЕАЛЬНУЮ БИБЛИОТЕКУ OPC UA
#define USE_REAL_OPCUA 1

#if USE_REAL_OPCUA
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#endif

class OPCUAClient {
public:
    struct TagData {
        std::string name;
        std::string nodeId;
        double value;
        std::string unit;
        std::string timestamp;
        std::string quality;
        bool is_written;
        
        TagData(const std::string& n, const std::string& id, const std::string& u)
            : name(n), nodeId(id), value(0.0), unit(u), quality("GOOD"), is_written(false) {}
        
        void update(double newValue, bool written = false);
        void updateTimestamp();
        void print() const;
    };
    
    struct TagHistory {
        std::vector<double> values;
        std::vector<std::string> timestamps;
        size_t maxHistory = 100;
        
        void addValue(double value, const std::string& timestamp);
        void clear();
    };
    
private:
#if USE_REAL_OPCUA
    UA_Client* client = nullptr;
#endif
    
    std::vector<TagData> tags;
    mutable std::mutex tags_mutex;
    std::string endpoint;
    bool connected = false;
    
    std::map<std::string, TagHistory> tagHistories;
    mutable std::mutex history_mutex;
    
    // Вспомогательные функции
    std::string nodeIdToString(const UA_NodeId* nodeId);
    std::string getNodeDisplayName(const UA_NodeId* nodeId);
    std::string getEngineeringUnits(const UA_NodeId* nodeId);
    void browseNodeInternal(const UA_NodeId* nodeId, int depth);
    
public:
    OPCUAClient();
    ~OPCUAClient();
    
    // Основные функции
    bool connect(const std::string& url);
    void disconnect();
    bool isConnected() const;
    
    bool discoverTags();
    void updateValues();
    
    bool writeTagByName(const std::string& tagName, double value);
    bool writeTagById(const std::string& nodeId, double value);
    
    bool resetTagToAuto(const std::string& tagName);
    void resetAllWrittenTags();
    
    // Геттеры
    std::vector<TagData> getTags() const;
    size_t tagCount() const;
    TagData* getTagByName(const std::string& tagName);
    TagHistory* getTagHistory(const std::string& tagName);
    
    void addTag(const std::string& name, const std::string& nodeId, 
                const std::string& unit, double minVal, double maxVal);
    
private:
    void addToHistory(const std::string& tagName, double value, const std::string& timestamp);
};