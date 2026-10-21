# Local

cmake_minimum_required(VERSION 3.22)
project(SteamSearch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

include(FetchContent)


FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

# Standalone (non-Boost) Asio, header-only. Crow's find_package(asio) looks for
# ASIO_INCLUDE_DIR, so we fetch the headers and point it there ourselves instead
# of requiring a system/vcpkg install.
FetchContent_Declare(asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-38-2
)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/include CACHE PATH "" FORCE)

set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(Crow
        GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
        GIT_TAG v1.3.5
)
FetchContent_MakeAvailable(Crow)

add_executable(data_converter src/converter.cpp)
target_link_libraries(data_converter nlohmann_json::nlohmann_json)

add_executable(steam_server src/mainServer.cpp)
target_link_libraries(steam_server
        Crow::Crow
        nlohmann_json::nlohmann_json)

if (WIN32)
    target_link_libraries(steam_server ws2_32 mswsock)
    # Statically link the MinGW runtime so the .exe doesn't depend on
    # libstdc++-6.dll / libgcc_s_seh-1.dll / libwinpthread-1.dll being on PATH
    # when run outside the IDE (e.g. double-clicked or launched from a plain
    # cmd/PowerShell window).
    target_link_options(data_converter PRIVATE -static-libgcc -static-libstdc++ -static)
    target_link_options(steam_server PRIVATE -static-libgcc -static-libstdc++ -static)
else ()
    target_link_libraries(steam_server pthread)
endif ()