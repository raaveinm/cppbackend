FROM gcc:11.3 as build

LABEL authors="raaveinm"

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

WORKDIR /app

COPY conanfile.txt .
RUN mkdir build && cd build && \
    conan install .. --build=missing

COPY ./src ./src
COPY CMakeLists.txt .

RUN cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_POLICY_DEFAULT_CMP0091=NEW && \
    cmake --build .

FROM ubuntu:22.04 as run

RUN groupadd -r www && useradd -r -g www www
USER www

WORKDIR /app

COPY --from=build /app/build/game_server .
COPY ./data ./data
COPY ./static ./static

ENTRYPOINT ["./game_server", "./data/config.json", "./static"]
