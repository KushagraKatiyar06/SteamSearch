# Local

cmake_minimum_required(VERSION 3.22)
project(SteamSearch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Command-line builds (CLion sets its own CMAKE_BUILD_TYPE via its CMake profile,
# so this only affects `cmake -S . -B ...` runs that don't pass -DCMAKE_BUILD_TYPE).
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

option(ENABLE_NATIVE_ARCH "Compile for this machine's CPU (-march=native). Local benchmarking only - do not use for binaries that run on other machines." OFF)
option(BUILD_BENCHMARKS "Build the bench_runner target" OFF)

if (ENABLE_NATIVE_ARCH AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    add_compile_options(-march=native)
endif ()

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

add_executable(steam_server src/mainServer.cpp src/GameStore.cpp)
target_link_libraries(steam_server
        Crow::Crow
        nlohmann_json::nlohmann_json)

if (BUILD_BENCHMARKS)
    add_executable(bench_runner bench/bench_main.cpp src/GameStore.cpp)
    target_include_directories(bench_runner PRIVATE src)
    target_link_libraries(bench_runner nlohmann_json::nlohmann_json)
    if (NOT WIN32)
        target_link_libraries(bench_runner pthread)
    endif ()

    # bench_runner exits non-zero if the optimized recommend path's output
    # ever diverges from the brute-force reference it's checked against.
    enable_testing()
    add_test(NAME recommend_topk_matches_reference COMMAND bench_runner WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    add_executable(minhash_eval bench/minhash_eval.cpp src/GameStore.cpp)
    target_include_directories(minhash_eval PRIVATE src)
    if (WIN32)
        target_link_options(minhash_eval PRIVATE -static-libgcc -static-libstdc++ -static)
    endif ()
endif ()

if (WIN32)
    target_link_libraries(steam_server ws2_32 mswsock)
    # Statically link the MinGW runtime so the .exe doesn't depend on
    # libstdc++-6.dll / libgcc_s_seh-1.dll / libwinpthread-1.dll being on PATH
    # when run outside the IDE (e.g. double-clicked or launched from a plain
    # cmd/PowerShell window).
    target_link_options(data_converter PRIVATE -static-libgcc -static-libstdc++ -static)
    target_link_options(steam_server PRIVATE -static-libgcc -static-libstdc++ -static)
    if (BUILD_BENCHMARKS)
        target_link_options(bench_runner PRIVATE -static-libgcc -static-libstdc++ -static)
    endif ()
else ()
    target_link_libraries(steam_server pthread)
endif ()