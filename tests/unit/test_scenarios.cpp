#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "../../include/opcua_client.hpp"

// Фикстура для сценариев
class ScenarioTest : public ::testing::Test {
protected:
    void SetUp() override {
        #ifndef USE_REAL_OPCUA
        client = std::make_unique<OPCUAClient>();
        client->connect("opc.tcp://localhost:4840");
        #else
        GTEST_SKIP() << "Требуется симуляционный режим";
        #endif
    }
    
    void TearDown() override {
        if (client) {
            client->disconnect();
        }
    }
    
    std::unique_ptr<OPCUAClient> client;
};

// Сценарий 1: Полный цикл работы с одним тегом
TEST_F(ScenarioTest, FullSingleTagWorkflow) {
    // 1. Добавляем тег
    client->addTag("ScenarioTag1", "ns=10;i=1", "units", 0, 100);
    
    // 2. Проверяем начальное состояние
    auto tag = client->getTagByName("ScenarioTag1");
    ASSERT_NE(tag, nullptr);
    EXPECT_FALSE(tag->is_written);
    EXPECT_EQ(tag->quality, "GOOD");
    
    // 3. Записываем значение
    bool writeResult = client->writeTagByName("ScenarioTag1", 50.0);
    EXPECT_TRUE(writeResult);
    EXPECT_TRUE(tag->is_written);
    EXPECT_DOUBLE_EQ(tag->value, 50.0);
    
    // 4. Сбрасываем в авто
    bool resetResult = client->resetTagToAuto("ScenarioTag1");
    EXPECT_TRUE(resetResult);
    EXPECT_FALSE(tag->is_written);
    
    // 5. Обновляем значения
    client->updateValues();
    
    // 6. Проверяем историю
    auto history = client->getTagHistory("ScenarioTag1");
    ASSERT_NE(history, nullptr);
    EXPECT_GT(history->values.size(), 0);
}

// Сценарий 2: Работа с несколькими тегами
TEST_F(ScenarioTest, MultipleTagsWorkflow) {
    const int TAG_COUNT = 5;
    
    // Добавляем несколько тегов
    for (int i = 0; i < TAG_COUNT; ++i) {
        client->addTag("MultiTag_" + std::to_string(i),
                      "ns=10;i=" + std::to_string(i),
                      "unit_" + std::to_string(i),
                      i * 10, i * 10 + 10);
    }
    
    EXPECT_EQ(client->tagCount(), TAG_COUNT);
    
    // Записываем значения в четные теги
    for (int i = 0; i < TAG_COUNT; i += 2) {
        client->writeTagByName("MultiTag_" + std::to_string(i), i * 10 + 5);
    }
    
    // Проверяем, какие теги в WRITTEN режиме
    auto tags = client->getTags();
    for (size_t i = 0; i < tags.size(); ++i) {
        if (i % 2 == 0) {
            EXPECT_TRUE(tags[i].is_written);
        } else {
            EXPECT_FALSE(tags[i].is_written);
        }
    }
    
    // Сбрасываем все
    client->resetAllWrittenTags();
    
    // Проверяем, что все в AUTO
    tags = client->getTags();
    for (const auto& tag : tags) {
        EXPECT_FALSE(tag.is_written);
    }
}

// Сценарий 3: Длительная работа с автообновлением
TEST_F(ScenarioTest, LongRunningWithAutoUpdate) {
    client->addTag("LongRunningTag", "ns=10;i=100", "units", 0, 100);
    
    // Выполняем несколько циклов обновления
    const int CYCLES = 10;
    std::vector<double> lastValues;
    
    for (int i = 0; i < CYCLES; ++i) {
        client->updateValues();
        
        auto tag = client->getTagByName("LongRunningTag");
        ASSERT_NE(tag, nullptr);
        
        lastValues.push_back(tag->value);
        
        // Небольшая задержка между обновлениями
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Проверяем, что значения менялись (в симуляции они случайные)
    bool valuesChanged = false;
    for (size_t i = 1; i < lastValues.size(); ++i) {
        if (lastValues[i] != lastValues[i - 1]) {
            valuesChanged = true;
            break;
        }
    }
    
    // В симуляционном режиме значения должны меняться
    EXPECT_TRUE(valuesChanged);
    
    // Проверяем историю
    auto history = client->getTagHistory("LongRunningTag");
    ASSERT_NE(history, nullptr);
    EXPECT_GE(history->values.size(), CYCLES);
}

// Сценарий 4: Обработка ошибок и граничные случаи
TEST_F(ScenarioTest, ErrorHandlingAndEdgeCases) {
    // 1. Пустые строки в тегах
    client->addTag("", "ns=10;i=1", "", 0, 0);
    EXPECT_EQ(client->tagCount(), 1);
    
    // 2. Очень большие/маленькие значения
    client->addTag("BigValues", "ns=10;i=2", "units", -1e6, 1e6);
    bool writeResult = client->writeTagByName("BigValues", 999999.999);
    EXPECT_TRUE(writeResult);
    
    // 3. Одинаковые min/max
    client->addTag("ZeroRange", "ns=10;i=3", "units", 42.0, 42.0);
    auto tag = client->getTagByName("ZeroRange");
    ASSERT_NE(tag, nullptr);
    EXPECT_DOUBLE_EQ(tag->value, 42.0);
    
    // 4. Отрицательные значения
    client->addTag("Negative", "ns=10;i=4", "units", -100, 0);
    writeResult = client->writeTagByName("Negative", -50.0);
    EXPECT_TRUE(writeResult);
    
    // 5. Очень длинное имя
    std::string longName(1000, 'X');
    client->addTag(longName, "ns=10;i=5", "units", 0, 100);
    EXPECT_EQ(client->tagCount(), 5);
}

// Сценарий 5: Производительность
TEST_F(ScenarioTest, PerformanceBenchmark) {
    const int TAG_COUNT = 100;
    
    auto startAdd = std::chrono::high_resolution_clock::now();
    
    // Добавляем много тегов
    for (int i = 0; i < TAG_COUNT; ++i) {
        client->addTag("PerfTag_" + std::to_string(i),
                      "ns=20;i=" + std::to_string(i),
                      "units", 0, 100);
    }
    
    auto endAdd = std::chrono::high_resolution_clock::now();
    
    auto startUpdate = std::chrono::high_resolution_clock::now();
    
    // Обновляем все значения
    client->updateValues();
    
    auto endUpdate = std::chrono::high_resolution_clock::now();
    
    // Вычисляем время
    auto addDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endAdd - startAdd);
    auto updateDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endUpdate - startUpdate);
    
    // Выводим результаты (видно при запуске с --gtest_print_time)
    std::cout << "\n=== Производительность ===" << std::endl;
    std::cout << "Добавление " << TAG_COUNT << " тегов: " << addDuration.count() << " мс" << std::endl;
    std::cout << "Обновление " << TAG_COUNT << " тегов: " << updateDuration.count() << " мс" << std::endl;
    std::cout << "Среднее время на тег (добавление): " << (addDuration.count() * 1.0 / TAG_COUNT) << " мс" << std::endl;
    std::cout << "Среднее время на тег (обновление): " << (updateDuration.count() * 1.0 / TAG_COUNT) << " мс" << std::endl;
    
    // Проверяем, что производительность приемлемая
    // Эти значения можно настроить под ваше железо
    EXPECT_LT(addDuration.count(), 5000);   // Менее 5 секунд на добавление
    EXPECT_LT(updateDuration.count(), 2000); // Менее 2 секунд на обновление
}

// Сценарий 6: Восстановление