cmake_minimum_required(VERSION 3.22)
project(SteamSearch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(json)

find_path(ASIO_INCLUDE_DIR NAMES asio.hpp)
if(NOT ASIO_INCLUDE_DIR)
    message(FATAL_ERROR "Asio headers not found. Ensure libasio-dev is installed.")
endif()

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${ASIO_INCLUDE_DIR})
target_compile_definitions(asio INTERFACE ASIO_STANDALONE)

find_package(Crow CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_executable(steam_server src/mainServer.cpp)

target_link_libraries(steam_server
        PRIVATE
        Crow::Crow
        nlohmann_json::nlohmann_json
        asio
        Threads::Threads
)
