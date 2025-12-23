#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include "../../include/opcua_client.hpp"

using ::testing::_;
using ::testing::Return;

class OPCUAClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Убедимся, что тестируем симуляционный режим
        #ifndef USE_REAL_OPCUA
        client = std::make_unique<OPCUAClient>();
        #else
        GTEST_SKIP() << "Тесты требуют симуляционного режима (отключите USE_REAL_OPCUA)";
        #endif
    }
    
    void TearDown() override {
        if (client) {
            client->disconnect();
        }
    }
    
    std::unique_ptr<OPCUAClient> client;
};

// Тест 1: Проверка конструктора
TEST_F(OPCUAClientTest, ConstructorInitializesCorrectly) {
    ASSERT_NE(client, nullptr);
    EXPECT_FALSE(client->isConnected());
    EXPECT_EQ(client->tagCount(), 0);
}

// Тест 2: Подключение в симуляционном режиме
TEST_F(OPCUAClientTest, ConnectInSimulationMode) {
    bool result = client->connect("opc.tcp://localhost:4840");
    EXPECT_TRUE(result);
    EXPECT_TRUE(client->isConnected());
}

// Тест 3: Добавление тегов
TEST_F(OPCUAClientTest, AddTagIncreasesCount) {
    size_t initialCount = client->tagCount();
    
    client->addTag("Temperature", "ns=2;i=1", "°C", 20.0, 30.0);
    
    EXPECT_EQ(client->tagCount(), initialCount + 1);
}

// Тест 4: Поиск тега по имени
TEST_F(OPCUAClientTest, GetTagByNameReturnsCorrectTag) {
    client->addTag("Pressure", "ns=2;i=2", "bar", 1.0, 2.0);
    
    auto tag = client->getTagByName("Pressure");
    
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->name, "Pressure");
    EXPECT_EQ(tag->nodeId, "ns=2;i=2");
    EXPECT_EQ(tag->unit, "bar");
}

// Тест 5: Запись значения в тег
TEST_F(OPCUAClientTest, WriteTagByNameUpdatesValue) {
    client->addTag("Voltage", "ns=2;i=3", "V", 220.0, 240.0);
    
    bool result = client->writeTagByName("Voltage", 230.0);
    
    EXPECT_TRUE(result);
    
    auto tag = client->getTagByName("Voltage");
    ASSERT_NE(tag, nullptr);
    EXPECT_DOUBLE_EQ(tag->value, 230.0);
    EXPECT_TRUE(tag->is_written);
    EXPECT_EQ(tag->quality, "GOOD");
}

// Тест 6: Сброс тега в автоматический режим
TEST_F(OPCUAClientTest, ResetTagToAutoChangesMode) {
    client->addTag("FlowRate", "ns=2;i=4", "m³/h", 40.0, 60.0);
    client->writeTagByName("FlowRate", 50.0);
    
    auto tag = client->getTagByName("FlowRate");
    ASSERT_NE(tag, nullptr);
    EXPECT_TRUE(tag->is_written);
    
    bool resetResult = client->resetTagToAuto("FlowRate");
    
    EXPECT_TRUE(resetResult);
    EXPECT_FALSE(tag->is_written);
}

// Тест 7: Обновление значений
TEST_F(OPCUAClientTest, UpdateValuesChangesTagValues) {
    client->addTag("Level", "ns=2;i=5", "%", 70.0, 80.0);
    
    auto tagBefore = client->getTagByName("Level");
    ASSERT_NE(tagBefore, nullptr);
    double initialValue = tagBefore->value;
    
    client->updateValues();
    
    auto tagAfter = client->getTagByName("Level");
    ASSERT_NE(tagAfter, nullptr);
    
    // В симуляционном режиме значения должны измениться
    EXPECT_NE(tagAfter->value, initialValue);
}

// Тест 8: Получение всех тегов
TEST_F(OPCUAClientTest, GetTagsReturnsAllTags) {
    client->addTag("Tag1", "ns=2;i=1", "unit1", 0, 10);
    client->addTag("Tag2", "ns=2;i=2", "unit2", 0, 20);
    client->addTag("Tag3", "ns=2;i=3", "unit3", 0, 30);
    
    auto tags = client->getTags();
    
    EXPECT_EQ(tags.size(), 3);
    EXPECT_EQ(tags[0].name, "Tag1");
    EXPECT_EQ(tags[1].name, "Tag2");
    EXPECT_EQ(tags[2].name, "Tag3");
}

// Тест 9: Обнаружение тегов
TEST_F(OPCUAClientTest, DiscoverTagsInSimulationMode) {
    bool result = client->discoverTags();
    
    EXPECT_TRUE(result);
    EXPECT_GT(client->tagCount(), 0);
}

// Тест 10: Сброс всех записанных тегов
TEST_F(OPCUAClientTest, ResetAllWrittenTags) {
    client->addTag("TagA", "ns=2;i=1", "unit", 0, 100);
    client->addTag("TagB", "ns=2;i=2", "unit", 0, 100);
    client->addTag("TagC", "ns=2;i=3", "unit", 0, 100);
    
    client->writeTagByName("TagA", 50.0);
    client->writeTagByName("TagB", 60.0);
    
    // TagC остается в AUTO режиме
    
    client->resetAllWrittenTags();
    
    auto tags = client->getTags();
    
    for (const auto& tag : tags) {
        EXPECT_FALSE(tag.is_written);
    }
}

// Тест 11: Получение истории тега
TEST_F(OPCUAClientTest, GetTagHistoryReturnsValidPointer) {
    client->addTag("Temperature", "ns=2;i=1", "°C", 20, 30);
    
    // Обновляем несколько раз для создания истории
    client->updateValues();
    client->updateValues();
    client->updateValues();
    
    auto history = client->getTagHistory("Temperature");
    
    ASSERT_NE(history, nullptr);
    EXPECT_GT(history->values.size(), 0);
}

// Тест 12: Обработка несуществующего тега
TEST_F(OPCUAClientTest, NonExistentTagOperationsFail) {
    // Попытка записи в несуществующий тег
    bool writeResult = client->writeTagByName("NonExistent", 100.0);
    EXPECT_FALSE(writeResult);
    
    // Попытка сброса несуществующего тега
    bool resetResult = client->resetTagToAuto("NonExistent");
    EXPECT_FALSE(resetResult);
    
    // Поиск несуществующего тега
    auto tag = client->getTagByName("NonExistent");
    EXPECT_EQ(tag, nullptr);
    
    // Получение истории несуществующего тега
    auto history = client->getTagHistory("NonExistent");
    EXPECT_EQ(history, nullptr);
}

// Тест 13: Потокобезопасность (базовый)
TEST_F(OPCUAClientTest, ThreadSafetyBasic) {
    // Создаем несколько тегов
    for (int i = 0; i < 10; ++i) {
        client->addTag("ThreadTag_" + std::to_string(i), 
                      "ns=2;i=" + std::to_string(i),
                      "unit", 0, 100);
    }
    
    // Запускаем несколько потоков с операциями чтения
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([this]() {
            for (int j = 0; j < 100; ++j) {
                auto tags = client->getTags();
                size_t count = client->tagCount();
                // Не должно быть крешей
                EXPECT_GE(count, 0);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Проверяем, что данные не повреждены
    EXPECT_EQ(client->tagCount(), 10);
}

// Тест 14: Работа с пустым списком тегов
TEST_F(OPCUAClientTest, EmptyTagListOperations) {
    // Должно быть 0 тегов
    EXPECT_EQ(client->tagCount(), 0);
    
    // Получение пустого списка
    auto tags = client->getTags();
    EXPECT_TRUE(tags.empty());
    
    // Обновление значений при пустом списке (не должно падать)
    EXPECT_NO_THROW({
        client->updateValues();
    });
}

// Тест 15: Многократное добавление тега с одним именем
TEST_F(OPCUAClientTest, DuplicateTagNameHandling) {
    client->addTag("Duplicate", "ns=2;i=1", "unit1", 0, 10);
    client->addTag("Duplicate", "ns=2;i=2", "unit2", 0, 20);
    
    // Должно добавиться два тега с одинаковым именем
    EXPECT_EQ(client->tagCount(), 2);
    
    // getTagByName вернет первый найденный
    auto tag = client->getTagByName("Duplicate");
    ASSERT_NE(tag, nullptr);
    EXPECT_EQ(tag->nodeId, "ns=2;i=1");
}