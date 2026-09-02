FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    ninja-build \
    python3-pip \
    git \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install conan==2.31.1

WORKDIR /app
COPY conanfile.txt .

RUN conan profile detect --force
RUN conan install . --output-folder=build --build=missing \
    -s build_type=Release -s compiler.cppstd=20 \
    -c "tools.build:cflags=['-std=gnu17']"

COPY . .

RUN cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/app/build/conan_toolchain.cmake \
    -DCMAKE_PREFIX_PATH=/app/build

RUN cmake --build build --target game_server

FROM ubuntu:22.04 AS runner

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    libpq5 \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install prometheus_client

WORKDIR /app

COPY --from=builder /app/build/game_server ./game_server
COPY --from=builder /app/data ./data
COPY --from=builder /app/static ./static
COPY web_exporter.py .

EXPOSE 8080 9200

CMD ["sh", "-c", "./game_server -c data/config.json -w static -t 50 | python3 -u web_exporter.py"]