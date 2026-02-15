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

RUN mkdir build && cd build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DSTANDALONE_MODE=OFF \
    && make -j$(nproc)

# Stage 2: Runtime
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 \
    libcurl4 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd -r iotedge && useradd -r -g iotedge iotedge

COPY --from=builder /app/build/data_filter /usr/local/bin/data_filter

USER iotedge

ENV TEMP_MIN_VALID=-40.0
ENV TEMP_MAX_VALID=85.0
ENV NOISE_THRESHOLD=0.5

ENTRYPOINT ["data_filter"]
