FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y \
    git build-essential cmake curl zip unzip tar pkg-config

WORKDIR /app

COPY . .

RUN git clone https://github.com/microsoft/vcpkg.git && \
    ./vcpkg/bootstrap-vcpkg.sh

RUN ./vcpkg/vcpkg install crow asio

RUN cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/app/vcpkg/scripts/buildsystems/vcpkg.cmake && \
    cmake --build build --config Release

FROM ubuntu:22.04

WORKDIR /app
COPY --from=builder /app/build/SteamSearch /app/
COPY public /app/public
COPY games.json /app/games.json
COPY tags.txt /app/tags.txt
COPY decoder.txt /app/decoder.txt

EXPOSE 8080
CMD ["./SteamSearch"]
