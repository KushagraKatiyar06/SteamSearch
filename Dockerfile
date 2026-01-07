FROM gcc:latest

RUN apt-get update && apt-get install -y \
    cmake \
    libasio-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make steam_server

EXPOSE 8080

CMD ["./build/steam_server"]