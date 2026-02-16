# ─── Multi-stage build for minimal production image ───

# Stage 1: Build
FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    libssl-dev \
    libcurl4-openssl-dev \
    uuid-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir -p build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DSTANDALONE_MODE=OFF \
    && make -j$(nproc)

# Stage 2: Runtime
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 \
    libcurl4 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Run as non-root for security
RUN groupadd -r iotedge && useradd -r -g iotedge iotedge

COPY --from=builder /app/build/sensor_simulator /usr/local/bin/sensor_simulator

USER iotedge

ENV TELEMETRY_INTERVAL_MS=3000
ENV SENSOR_ID=temp-sensor-001

ENTRYPOINT ["sensor_simulator"]
