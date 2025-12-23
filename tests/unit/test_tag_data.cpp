#include <gtest/gtest.h>
#include <sstream>
#include "../../include/opcua_client.hpp"

// Тесты для структуры TagData
TEST(TagDataTest, ConstructorSetsCorrectValues) {
    OPCUAClient::TagData tag("Temperature", "ns=2;i=1", "°C");
    
    EXPECT_EQ(tag.name, "Temperature");
    EXPECT_EQ(tag.nodeId, "ns=2;i=1");
    EXPECT_EQ(tag.unit, "°C");
    EXPECT_DOUBLE_EQ(tag.value, 0.0);
    EXPECT_FALSE(tag.is_written);
    EXPECT_EQ(tag.quality, "BAD"); // При value = 0.0
    EXPECT_FALSE(tag.timestamp.empty());
}

TEST(TagDataTest, UpdateChangesValueAndTimestamp) {
    OPCUAClient::TagData tag("Pressure", "ns=2;i=2", "bar");
    
    std::string oldTimestamp = tag.timestamp;
    
    // Ждем, чтобы гарантировать изменение времени
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    tag.update(1.5, false);
    
    EXPECT_DOUBLE_EQ(tag.value, 1.5);
    EXPECT_FALSE(tag.is_written);
    EXPECT_EQ(tag.quality, "GOOD");
    EXPECT_NE(tag.timestamp, oldTimestamp);
}

TEST(TagDataTest, UpdateWithWriteFlagSetsWritten) {
    OPCUAClient::TagData tag("Voltage", "ns=2;i=3", "V");
    
    tag.update(230.0, true);
    
    EXPECT_DOUBLE_EQ(tag.value, 230.0);
    EXPECT_TRUE(tag.is_written);
    EXPECT_EQ(tag.quality, "GOOD");
}

TEST(TagDataTest, PrintFunctionOutputFormat) {
    OPCUAClient::TagData tag("TestTag", "ns=2;i=1", "units");
    tag.update(42.5, false);
    
    // Перенаправляем std::cout для проверки
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    
    tag.print();
    
    std::cout.rdbuf(old);
    std::string output = buffer.str();
    
    // Проверяем, что вывод содержит ожидаемые данные
    EXPECT_TRUE(output.find("TestTag") != std::string::npos);
    EXPECT_TRUE(output.find("42.5") != std::string::npos);
    EXPECT_TRUE(output.find("units") != std::string::npos);
    EXPECT_TRUE(output.find("[AUTO]") != std::string::npos);
}

TEST(TagDataTest, QualityChangesBasedOnValue) {
    OPCUAClient::TagData tag("Test", "ns=2;i=1", "unit");
    
    // Начальное значение 0.0 -> BAD
    EXPECT_EQ(tag.quality, "BAD");
    
    // Любое ненулевое значение -> GOOD
    tag.update(0.1, false);
    EXPECT_EQ(tag.quality, "GOOD");
    
    tag.update(-0.1, false);
    EXPECT_EQ(tag.quality, "GOOD");
    
    tag.update(0.0, false);
    EXPECT_EQ(tag.quality, "BAD");
}

// Тесты для TagHistory
TEST(TagHistoryTest, AddValueUpdatesVectors) {
    OPCUAClient::TagHistory history;
    
    EXPECT_EQ(history.values.size(), 0);
    EXPECT_EQ(history.timestamps.size(), 0);
    
    history.addValue(10.0, "12:00:00");
    
    EXPECT_EQ(history.values.size(), 1);
    EXPECT_EQ(history.timestamps.size(), 1);
    EXPECT_DOUBLE_EQ(history.values[0], 10.0);
    EXPECT_EQ(history.timestamps[0], "12:00:00");
}

TEST(TagHistoryTest, MaxHistoryLimitsSize) {
    OPCUAClient::TagHistory history;
    history.maxHistory = 3; // Ограничиваем размер
    
    // Добавляем 5 значений
    history.addValue(1.0, "12:00:00");
    history.addValue(2.0, "12:00:01");
    history.addValue(3.0, "12:00:02");
    history.addValue(4.0, "12:00:03");
    history.addValue(5.0, "12:00:04");
    
    // Должно остаться только последние 3
    EXPECT_EQ(history.values.size(), 3);
    EXPECT_EQ(history.timestamps.size(), 3);
    
    // Проверяем значения
    EXPECT_DOUBLE_EQ(history.values[0], 3.0);
    EXPECT_DOUBLE_EQ(history.values[1], 4.0);
    EXPECT_DOUBLE_EQ(history.values[2], 5.0);
    
    EXPECT_EQ(history.timestamps[0], "12:00:02");
    EXPECT_EQ(history.timestamps[1], "12:00:03");
    EXPECT_EQ(history.timestamps[2], "12:00:04");
}

TEST(TagHistoryTest, ClearRemovesAllData) {
    OPCUAClient::TagHistory history;
    
    // Добавляем данные
    history.addValue(10.0, "12:00:00");
    history.addValue(20.0, "12:00:01");
    history.addValue(30.0, "12:00:02");
    
    EXPECT_EQ(history.values.size(), 3);
    EXPECT_EQ(history.timestamps.size(), 3);
    
    // Очищаем
    history.clear();
    
    EXPECT_EQ(history.values.size(), 0);
    EXPECT_EQ(history.timestamps.size(), 0);
}

TEST(TagHistoryTest, DefaultMaxHistoryIs100) {
    OPCUAClient::TagHistory history;
    EXPECT_EQ(history.maxHistory, 100);
}

TEST(TagHistoryTest, AddMultipleValuesMaintainsOrder) {
    OPCUAClient::TagHistory history;
    
    for (int i = 0; i < 10; ++i) {
        history.addValue(i * 1.0, "time_" + std::to_string(i));
    }
    
    EXPECT_EQ(history.values.size(), 10);
    EXPECT_EQ(history.timestamps.size(), 10);
    
    // Проверяем порядок
    for (int i = 0; i < 10; ++i) {
        EXPECT_DOUBLE_EQ(history.values[i], i * 1.0);
        EXPECT_EQ(history.timestamps[i], "time_" + std::to_string(i));
    }
}