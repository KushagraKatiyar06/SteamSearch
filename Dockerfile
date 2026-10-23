# Use a modern GCC image
FROM gcc:latest

# cmake + git are all that's needed - CMakeLists.txt fetches Crow, Asio, and
# nlohmann/json itself via FetchContent.
RUN apt-get update && apt-get install -y \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --target steam_server

EXPOSE 8080

CMD ["./steam_server"]