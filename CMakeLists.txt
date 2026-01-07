cmake_minimum_required(VERSION 3.22)
project(SteamSearch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

find_package(asio REQUIRED)
find_package(Crow CONFIG REQUIRED)

add_executable(steam_server mainServer.cpp)

find_package(Threads REQUIRED)
target_link_libraries(steam_server
        PRIVATE
        Crow::Crow
        nlohmann_json::nlohmann_json
        Threads::Threads
)

add_executable(data_converter converter.cpp)
target_link_libraries(data_converter
        PRIVATE
        nlohmann_json::nlohmann_json
)