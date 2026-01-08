cmake_minimum_required(VERSION 3.22)
project(SteamSearch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

include(FetchContent)


FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)


find_package(Crow CONFIG REQUIRED)

add_executable(data_converter src/converter.cpp)
target_link_libraries(data_converter nlohmann_json::nlohmann_json)

add_executable(steam_server src/mainServer.cpp)
target_link_libraries(steam_server
        Crow::Crow
        nlohmann_json::nlohmann_json
        pthread)