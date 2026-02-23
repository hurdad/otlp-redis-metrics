# syntax=docker/dockerfile:1.7

FROM ubuntu:24.04 AS builder

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    libprotobuf-dev \
    protobuf-compiler \
    protobuf-compiler-grpc \
    libgrpc++-dev \
    libhiredis-dev \
    git \
    ca-certificates \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
 && cmake --build build --target otlp-redis-metrics

FROM ubuntu:24.04 AS runtime

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    libgrpc++1.51t64 \
    libprotobuf32t64 \
    libhiredis1.1.0 \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /src/build/otlp-redis-metrics /usr/local/bin/otlp-redis-metrics

EXPOSE 4317
ENTRYPOINT ["/usr/local/bin/otlp-redis-metrics"]
