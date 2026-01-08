# Steam Search | A Game Recommendation Engine

Steam Search is a game recommendation engine which has access to over 100,000 games from the Steam game launcher. I utilized indexing and binary files to condense a 600mb json into a 125mb 
file. This reduces the ram overhead for hosting services from 1gb to just 300 mb. The algorithms are also implemented on C++ to maintain high performance and results in sub 200ms runtimes for recommendations.

Here it is: https://steamsearch-production-622f.up.railway.app/

# Tech Stack

Frontend: React, HTML, CSS, Javascript

Backend: C++, Crow,

Dev Ops: Docker

# Setup

1. The backend utilizes the Crow library so you will need to clone the vcpkgk repository: https://github.com/microsoft/vcpkg
2. I used Clion so I go into the CMake settings and link the vcpkge folder. Here is what to do in the cmake:

```
# Add these for Windows/vcpkg 
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(ASIO_INCLUDE_DIR "C:/vcpkg/installed/x64-windows/include")

# Link Windows-specific libraries
target_link_libraries(steam_server
        Crow::Crow
        nlohmann_json::nlohmann_json
        ws2_32
        mswsock)
```

3. The frontend is pretty straightforward, just npm install and npm run dev in the frontend folder.

### Developed by Kushagra Katiyar
 
