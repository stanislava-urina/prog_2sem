cmake_minimum_required(VERSION 3.14)
project(OPCUA_TUI_Project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Отключаем предупреждения MSVC о безопасности
add_definitions(-D_CRT_SECURE_NO_WARNINGS)

# --- Зависимости через FetchContent ---
include(FetchContent)

FetchContent_Declare(ftxui GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI GIT_TAG v5.0.0)
FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest GIT_TAG v1.14.0)

FetchContent_MakeAvailable(ftxui googletest)

# --- Подключение open62541 ---
add_subdirectory(open62541)
add_definitions(-DUSE_REAL_OPCUA)

# --- Библиотека логики (Shared Logic) ---
add_library(opcua_logic src/opcua_client.cpp)
target_link_libraries(opcua_logic PUBLIC open62541)

if(WIN32)
    target_link_libraries(opcua_logic PUBLIC ws2_32 advapi32 user32 gdi32)
endif()

target_include_directories(opcua_logic PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/open62541/include
    ${CMAKE_CURRENT_BINARY_DIR}/open62541/src_generated
)

# --- Основное приложение ---
add_executable(opcua_monitor src/main.cpp)
target_link_libraries(opcua_monitor PRIVATE 
    opcua_logic 
    ftxui::screen 
    ftxui::dom 
    ftxui::component
)

# --- ТЕСТЫ ---
enable_testing()
add_executable(client_tests tests/test_client.cpp)
target_link_libraries(client_tests PRIVATE 
    opcua_logic 
    GTest::gtest_main
    ftxui::screen 
    ftxui::dom 
    ftxui::component
)

include(GoogleTest)
gtest_discover_tests(client_tests)