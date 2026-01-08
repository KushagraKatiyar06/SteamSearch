# Local

cmake_minimum_required(VERSION 3.22)
project(SteamSearch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

include(FetchContent)

# nlohmann_json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# Crow and Asio
set(ASIO_INCLUDE_DIR "C:/vcpkg/installed/x64-mingw-static/include")
set(Crow_DIR "C:/vcpkg/installed/x64-mingw-static/share/crow")

list(APPEND CMAKE_PREFIX_PATH "C:/vcpkg/installed/x64-mingw-static")
find_package(Crow CONFIG REQUIRED)

add_executable(data_converter src/converter.cpp)
target_link_libraries(data_converter
        nlohmann_json::nlohmann_json)

add_executable(steam_server src/mainServer.cpp)
target_link_libraries(steam_server
        Crow::Crow
        nlohmann_json::nlohmann_json
        ws2_32
        mswsock)