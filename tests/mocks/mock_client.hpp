#pragma once

#include <gmock/gmock.h>
#include "../../include/opcua_client.hpp"

// Mock для тестирования GUI без реального OPC UA
class MockOPCUAClient : public OPCUAClient {
public:
    MOCK_METHOD(bool, connect, (const std::string& url), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(bool, isConnected, (), (const override));
    MOCK_METHOD(bool, discoverTags, (), (override));
    MOCK_METHOD(void, updateValues, (), (override));
    MOCK_METHOD(bool, writeTagByName, (const std::string& tagName, double value), (override));
    MOCK_METHOD(bool, writeTagById, (const std::string& nodeId, double value), (override));
    MOCK_METHOD(bool, resetTagToAuto, (const std::string& tagName), (override));
    MOCK_METHOD(void, resetAllWrittenTags, (), (override));
    MOCK_METHOD(std::vector<TagData>, getTags, (), (const override));
    MOCK_METHOD(size_t, tagCount, (), (const override));
    MOCK_METHOD(TagData*, getTagByName, (const std::string& tagName), (override));
    MOCK_METHOD(TagHistory*, getTagHistory, (const std::string& tagName), (override));
    MOCK_METHOD(void, addTag, 
                (const std::string& name, const std::string& nodeId, 
                 const std::string& unit, double minVal, double maxVal), 
                (override));
};