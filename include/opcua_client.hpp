#ifndef OPCUA_CLIENT_HPP
#define OPCUA_CLIENT_HPP

#include <string>
#include <vector>
#include <mutex>
#include <open62541/client_highlevel.h>
#include <open62541/client_config_default.h>

class OPCUAClient {
public:
    struct TagData {
        std::string name;
        std::string nodeId;
        double value;
        std::string timestamp;
        std::string quality;

        TagData(std::string n, std::string id) 
            : name(n), nodeId(id), value(0.0), quality("INIT") {}
    };

    OPCUAClient();
    ~OPCUAClient();

    bool connectToServer(const std::string& url);
    void disconnectFromServer();
    bool isConnected() const { return connected; }
    
    std::vector<TagData> getTags();
    void updateValues();
    bool writeValue(const std::string& nodeId, double newValue);

private:
    UA_Client *client;
    bool connected;
    std::vector<TagData> tags;
    std::mutex tags_mutex;
};

#endif