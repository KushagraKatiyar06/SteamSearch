cmake_minimum_required(VERSION 3.29)
project(SteamSearch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# -------------------------------
# Fetch dependencies
# -------------------------------
include(FetchContent)

# nlohmann_json
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

# RapidFuzz C++
FetchContent_Declare(
        rapidfuzz
        GIT_REPOSITORY https://github.com/rapidfuzz/rapidfuzz-cpp.git
        GIT_TAG main
)
FetchContent_MakeAvailable(rapidfuzz)

# Asio (header-only, cross-platform)
FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-27-0
)
FetchContent_MakeAvailable(asio)

# Crow
find_package(Crow CONFIG REQUIRED)

# -------------------------------
# Define executable
# -------------------------------
add_executable(SteamSearch
        main.cpp
        Game.h
        Game.cpp
        readJson.h
        readJson.cpp
        jaccardsSimilarity.h
        jaccardsSimilarity.cpp
        minHash.h
        minHash.cpp
        cosineSimilarity.h
        cosineSimilarity.cpp
        multiFeatureSimilarity.h
        multiFeatureSimilarity.cpp
        algorithms_B.h
        algorithms_B.cpp
        RapidFuzzie.h
        RapidFuzzie.cpp
        main.cpp
)

# -------------------------------
# Link libraries
# -------------------------------
target_link_libraries(SteamSearch PRIVATE
        Crow::Crow
        nlohmann_json::nlohmann_json
        rapidfuzz::rapidfuzz
        asio::asio
)

# -------------------------------
# Platform-specific libraries
# -------------------------------
if(WIN32)
    target_link_libraries(SteamSearch PRIVATE Ws2_32 Mswsock)
endif()
