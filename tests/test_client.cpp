#include <gtest/gtest.h>
#include "../include/opcua_client.hpp"

// Проверка начального состояния без подключения
TEST(OPCUAClientTest, InitialState) {
    OPCUAClient client;
    EXPECT_FALSE(client.isConnected());
}

// Проверка инициализации тегов
TEST(OPCUAClientTest, TagsInitialization) {
    OPCUAClient client;
    auto tags = client.getTags();
    
    ASSERT_EQ(tags.size(), 2);
    EXPECT_EQ(tags[0].name, "Temperature");
    EXPECT_EQ(tags[1].name, "Voltage");
}

// Проверка поведения при записи без сервера
TEST(OPCUAClientTest, WriteValueOffline) {
    OPCUAClient client;
    bool result = client.writeValue("ns=2;i=2", 10.5);
    EXPECT_FALSE(result);
}