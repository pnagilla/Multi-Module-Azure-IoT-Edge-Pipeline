# ─── Multi-stage build for Python analytics module ───

# Stage 1: Install dependencies
FROM python:3.11-slim AS builder

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir --prefix=/install -r requirements.txt

# Stage 2: Runtime
FROM python:3.11-slim

RUN groupadd -r iotedge && useradd -r -g iotedge iotedge

WORKDIR /app

# Copy installed packages from builder
COPY --from=builder /install /usr/local

COPY src/ ./

USER iotedge

ENV ALERT_TEMP_HIGH=35.0
ENV ALERT_TEMP_LOW=-10.0
ENV ROLLING_WINDOW_SIZE=10

ENTRYPOINT ["python", "-u", "main.py"]
