cmake_minimum_required(VERSION 3.22)
project(SteamSearch)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# nlohmann_json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# Crow and Asio - Let CMake find these on the system (set in Dockerfile)
find_package(asio REQUIRED)
find_package(Crow CONFIG REQUIRED)

# Executables
add_executable(steam_server mainServer.cpp)
target_link_libraries(steam_server
        Crow::Crow
        nlohmann_json::nlohmann_json
        pthread) # Added for Linux multithreading