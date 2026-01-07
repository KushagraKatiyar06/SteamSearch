FROM gcc:latest

RUN apt-get update && apt-get install -y \
    cmake \
    libasio-dev \
    git \
    git-lfs \
    && rm -rf /var/lib/apt/lists/*


RUN git clone https://github.com/CrowCpp/Crow.git /tmp/crow && \
    cd /tmp/crow && \
    mkdir build && cd build && \
    cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF && \
    make install && \
    rm -rf /tmp/crow

WORKDIR /app

COPY . .

RUN git lfs install && git lfs pull

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --target steam_server

EXPOSE 8080

CMD ["./build/steam_server"]