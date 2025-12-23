cmake_minimum_required(VERSION 3.10)
project(opcua_gui)

# ★★★ ДОБАВЛЯЕМ ЭТО ДЛЯ GOOGLE TEST ★★★
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    # Автоматически скачиваем Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    FetchContent_MakeAvailable(googletest)
endif()
# ★★★ КОНЕЦ ДОБАВЛЕНИЯ ★★★

if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE TRUE)
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ★★★ ПРАВИЛЬНО подключаем open62541 ★★★
add_subdirectory(open62541)
add_definitions(-DUSE_REAL_OPCUA)

# ★★★ Твой исполняемый файл ★★★
add_executable(opcua_gui WIN32
    src/simple_opcua_gui.cpp
    src/opcua_client.cpp
    src/graph_window.cpp
    src/graph_renderer.cpp
    src/write_dialog.cpp
    resource.rc
)

# ★★★ ДОБАВЛЯЕМ ЭТО ДЛЯ ТЕСТОВ ★★★
if(BUILD_TESTS)
    # Подключаем папку с тестами
    add_subdirectory(tests)
endif()
# ★★★ КОНЕЦ ДОБАВЛЕНИЯ ★★★

# ★★★ Связываем с библиотекой ★★★
target_link_libraries(opcua_gui PRIVATE open62541)

# ★★★ ВАЖНО: ПРАВИЛЬНЫЕ пути к заголовкам ★★★
target_include_directories(opcua_gui PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/open62541/include  # основной путь
)

# ★★★ Windows библиотеки ★★★
if(WIN32)
    target_link_libraries(opcua_gui PRIVATE 
        comctl32
        ws2_32
        advapi32
        user32
        gdi32
    )
endif()