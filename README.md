# otlp-redis-metrics

A small C++ service that receives OTLP Metrics over gRPC and writes them to RedisTimeSeries.

## Build (Ubuntu 24.04)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build pkg-config \
  libprotobuf-dev protobuf-compiler protobuf-compiler-grpc \
  libgrpc++-dev libhiredis-dev git

cmake -S . -B build -G Ninja
cmake --build build
```

## Example config JSON

```json
{
  "redis": {"host": "127.0.0.1", "port": 6379, "db": 0, "unixSocket": ""},
  "timeseries": {
    "keyPrefix": "metrics:",
    "retentionMs": "86400000",
    "createOnWrite": true,
    "ensureRules": true,
    "downsampleMs": ["60000", "300000"]
  },
  "ingest": {
    "maxQueueDepth": 50000,
    "maxBatchPoints": 5000,
    "flushIntervalMs": 200,
    "maxLabelKv": 12,
    "maxLabelValueLen": 128
  },
  "otlp": {"listenAddr": "0.0.0.0:4317", "grpcThreads": 2},
  "labelPolicy": {
    "allowedKeys": ["host", "service", "instance", "core", "gpu", "queue", "device"]
  },
  "normalizeMetricNames": false
}
```

Use `redis.unixSocket` to connect over a Unix domain socket. When `unixSocket` is set, `host` and `port` are ignored.

Example:

```json
{
  "redis": {"unixSocket": "/var/run/redis/redis.sock", "db": 0}
}
```

## Run

```bash
./build/otlp-redis-metrics --config_json=./config.json
```

Print effective config:

```bash
./build/otlp-redis-metrics --config_json=./config.json --print_effective_config=true
```

Run self test:

```bash
./build/otlp-redis-metrics --config_json=./config.json --self_test=true
```

## OTLP exporter environment

```bash
export OTEL_EXPORTER_OTLP_ENDPOINT=http://localhost:4317
export OTEL_EXPORTER_OTLP_PROTOCOL=grpc
```

## RedisTimeSeries inspection

```redis
TS.QUERYINDEX metric=system_cpu_utilization
TS.MRANGE - + FILTER metric=system_cpu_utilization
```

## Docker

Build image locally:

```bash
docker build -t otlp-redis-metrics:local .
```

Run:

```bash
docker run --rm -p 4317:4317 \
  -v "$PWD/config.json:/app/config.json:ro" \
  otlp-redis-metrics:local --config_json=/app/config.json
```

Run with Docker Compose (app + Redis Stack with RedisTimeSeries + Unix socket):

```bash
docker compose up --build
```


## GitHub Actions multi-arch image publish

This repo includes `.github/workflows/docker-image.yml` to build `linux/amd64` and `linux/arm64` images with Buildx.

- Pull requests: builds multi-arch image (no push).
- Pushes to `main` and `v*` tags: builds and pushes to `ghcr.io/<owner>/<repo>`.
