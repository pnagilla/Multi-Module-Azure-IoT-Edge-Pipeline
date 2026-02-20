# Multi-Module Azure IoT Edge Pipeline

> A production-style IoT Edge pipeline with 3 modules demonstrating edge computing, message routing, and cloud integration.

---

## What This Project Does (30-second pitch)

- Simulates a **cold storage temperature monitoring system**
- 3 modules run on an edge device, connected via **message routing**
- Only **filtered alerts** reach the cloud — raw data stays local
- Built with **C++ (sensor + filter)** and **Python (analytics)** in Docker containers

---

## Architecture at a Glance

```
[Sensor Simulator] --> [Data Filter] --> [Analytics & Alert] --> Azure IoT Hub
       C++                  C++               Python               (cloud)
   generates data      rejects garbage     computes stats        receives only
   every 3 sec         & spike data        & triggers alerts     alerts/summaries
```

- Modules **never talk directly** — all messages go through **Edge Hub** (local message broker)
- Routing is defined in a **deployment manifest** (JSON config), not in code
- Adding/removing a module = changing the manifest, **zero code changes** to other modules

---

## Key Concepts You Must Know

### 1. Why Edge Computing?

| Problem (without edge) | Solution (with edge) |
|---|---|
| 10,000 sensors x 1 msg/sec = 864M msgs/day to cloud | Process locally, send only what matters |
| Internet outage = data loss | Edge Hub **stores and forwards** when offline |
| Cloud round-trip = 100-500ms latency | Local decisions in <1ms |
| High bandwidth cost | 90%+ data reduction before cloud |

### 2. Azure IoT Components (3 pieces)

| Component | Where it runs | What it does |
|---|---|---|
| **IoT Hub** | Cloud | Device registry, receives D2C messages, sends C2D commands |
| **Edge Agent** | On device | Pulls container images, starts/stops modules, reports health |
| **Edge Hub** | On device | Routes messages between modules, buffers when offline (store-and-forward) |

### 3. Message Routing (most important for interviews)

```
Route 1:  Sensor output   -->  Filter input       (sensorToFilter)
Route 2:  Filter output   -->  Analytics input     (filterToAnalytics)
Route 3:  Analytics output -->  $upstream (cloud)  (alertsToHub)
```

- `$upstream` = send to IoT Hub in the cloud
- `BrokeredEndpoint("/modules/X/inputs/Y")` = send to another module
- Routes support **conditions** (e.g., only route critical alerts)
- **TTL** (Time to Live) = how long Edge Hub stores messages offline (we set 2 hours)

### 4. Deployment Manifest

- A JSON file pushed to IoT Hub that tells the device:
  - **What modules** to run (container images)
  - **Environment variables** for each module
  - **Restart policy** (always restart on crash)
  - **Routes** (message flow between modules)
- Edge Agent polls for manifest changes and auto-applies them — **no SSH needed**

### 5. Module Twin

- A JSON document in IoT Hub with **desired** and **reported** properties
- Change config from cloud (e.g., alert threshold 35C -> 40C) without redeploying
- Module receives update, applies it live

---

## The 3 Modules

### Module 1: Sensor Simulator (C++)

- **Job**: Generate realistic temperature readings every 3 seconds
- **Technique**: Brownian-motion drift + Gaussian noise (mimics real sensor behavior)
- **Output**: JSON with `sensorId`, `temperature`, `humidity`, `timestamp`, `sequenceNumber`
- **Why C++**: Direct hardware access, minimal memory — critical for tiny edge devices

### Module 2: Data Filter (C++)

- **Job**: Reject invalid/garbage readings before analytics
- **Check 1 — Range validation**: Reject if temp < -40C or > 85C (physically impossible)
- **Check 2 — Spike detection**: Reject if value jumps abnormally (uses rolling mean + standard deviation)
- **Result**: ~9% of readings filtered out, only clean data passes through

### Module 3: Analytics & Alert (Python)

- **Job**: Compute rolling statistics and trigger alerts
- **Stats**: Mean, standard deviation, min, max, trend (over last 10 readings)
- **Alert levels**:
  - **CRITICAL**: temp > 35C or temp < -10C
  - **WARNING**: temp > 30C or temp < -5C, or rapid trend change
- **Why Python**: Best ecosystem for analytics, ML, and statistical libraries

---

## How We Run It

### Local Development (no Azure needed)

```bash
make build          # Build C++ modules
make run-local      # Runs: sensor | filter | analytics (Unix pipes)
```

- **Standalone mode**: Modules read stdin / write stdout
- This is the same code that runs in production — controlled by `STANDALONE_MODE` CMake flag

### With Azure IoT Hub

```bash
set -a && source .env && set +a
make run-local | python3 scripts/send-to-hub.py
```

- `send-to-hub.py` reads pipeline output and forwards to IoT Hub via device connection string
- Monitor in another terminal: `az iot hub monitor-events --hub-name <your-hub>`

### Docker

```bash
make docker         # Build all 3 container images
make run-pipeline   # Run via docker-compose
```

- **Multi-stage builds**: Build stage ~500MB -> Runtime stage ~50MB
- **Non-root containers**: All run as `iotedge` user
- `.dockerignore` prevents local build artifacts from leaking in

---

## Production Best Practices in This Project

| Practice | How we do it |
|---|---|
| **Dual-mode binaries** | Same code compiles for local dev (stdin/stdout) or Edge (IoT SDK) via `STANDALONE_MODE` flag |
| **Multi-stage Docker** | Build tools in stage 1, copy only binary to minimal stage 2 |
| **Non-root containers** | All Dockerfiles: `USER iotedge` |
| **Structured logging** | Status/errors to stderr, data to stdout |
| **Graceful shutdown** | SIGINT/SIGTERM signal handlers in all modules |
| **Store-and-forward** | Edge Hub configured with 2-hour TTL for offline resilience |
| **Configurable via env vars** | All thresholds, intervals, IDs are environment variables |
| **Unit tests** | Analytics engine has test coverage for all alert paths |

---

## Project vs Production

| Aspect | This Project | Production |
|---|---|---|
| Sensor | Simulated (random numbers) | Real hardware (Modbus, OPC-UA protocols) |
| Edge Device | Your Mac/laptop | Raspberry Pi, industrial gateway |
| Registry | localhost:5000 | Azure Container Registry (ACR) |
| Deployment | Manual `make run-local` | IoT Hub pushes manifest, auto-deploys |
| Scale | 1 device | 1000s of devices via deployment groups |
| Communication | Unix pipes (standalone mode) | Edge Hub MQTT routing |
| Security | Connection string | X.509 certificates, HSM |
| Monitoring | CLI commands | Azure Monitor, Time Series Insights |

**The architecture and code patterns are identical** — same modules run locally or on a real edge device with zero code changes.

---

## Interview One-Liner

> "I built a multi-module IoT Edge pipeline with a C++ sensor simulator, a C++ data filter for noise rejection and spike detection, and a Python analytics engine for threshold alerting. Modules communicate through Edge Hub routing, with only alerts forwarded upstream to IoT Hub. I used multi-stage Docker builds, dual-mode compilation for local development, and deployed end-to-end to Azure IoT Hub's free tier — achieving ~9% data reduction through edge filtering before any data reached the cloud."

---

## Quick Interview Q&A

**Q: What is Azure IoT Edge?**
A runtime on edge devices that runs cloud workloads as Docker containers (modules) locally, reducing latency, bandwidth, and enabling offline operation.

**Q: IoT Hub vs IoT Edge?**
Hub = cloud service (device registry, messaging). Edge = device runtime (local processing via containers). Edge Hub is a local mini-version of IoT Hub.

**Q: How do modules communicate?**
Never directly. All messages route through Edge Hub. Routes are defined in the deployment manifest (JSON). This decouples modules completely.

**Q: What happens when internet goes down?**
Edge Hub has store-and-forward. Messages queue locally (configurable TTL). When connectivity returns, they auto-forward to IoT Hub in order.

**Q: How do you deploy updates?**
Update the deployment manifest in IoT Hub. Edge Agent on each device polls for changes, pulls new images, restarts modules. No SSH.

**Q: What's a Module Twin?**
JSON config in IoT Hub with desired/reported properties. Change settings (e.g., alert threshold) from cloud without redeploying the container.

**Q: What protocols?**
MQTT (default, lightweight), AMQP (enterprise), HTTPS. Edge Hub supports all three.

**Q: Why not just use regular Docker?**
IoT Edge adds: remote deployment at scale, built-in store-and-forward, declarative routing, auto certificate rotation, and device health monitoring — things you'd have to build yourself with plain Docker.

---

## Key Terms Cheat Sheet

| Term | Meaning |
|---|---|
| **Edge Computing** | Process data near its source, not in cloud |
| **IoT Hub** | Cloud service managing devices and messages |
| **Edge Agent** | Downloads and manages containers on device |
| **Edge Hub** | Local message broker + offline buffer |
| **Module** | Docker container running on edge device |
| **Route** | Rule telling Edge Hub where to send messages |
| **$upstream** | "Send to cloud" keyword in routes |
| **D2C / C2D** | Device-to-Cloud / Cloud-to-Device messages |
| **Module Twin** | Remote config for a module (no redeploy) |
| **Store-and-Forward** | Queue messages locally when offline |
| **Deployment Manifest** | JSON defining modules + routes for a device |
| **MQTT** | Lightweight IoT messaging protocol |
| **OPC-UA / Modbus** | Industrial protocols for reading factory machines |
| **Connection String** | Credentials for device-to-hub authentication |
| **Multi-stage Build** | Docker: build in stage 1, copy binary to minimal stage 2 |
| **SKU F1** | Free tier IoT Hub (8K messages/day) |

---

## Where IoT Edge is Used

- **Manufacturing**: Predictive maintenance, quality control with camera + AI
- **Energy**: Oil pipeline monitoring, wind turbines, smart grid
- **Retail**: Smart checkout, refrigerator monitoring, foot traffic analytics
- **Healthcare**: Patient monitoring, local medical imaging AI (data privacy)
- **Transportation**: Fleet tracking, connected vehicles, railway monitoring
- **Video/AI**: Security cameras with local object detection, license plate recognition

---

## Project Structure

```
Multi-Module-Azure-IoT-Edge-Pipeline/
├── modules/
│   ├── sensor_simulator/    # C++ - temperature sensor simulation
│   ├── data_filter/         # C++ - range validation + spike detection
│   └── analytics_alert/     # Python - rolling stats + alerting
├── config/
│   └── deployment.template.json   # IoT Edge deployment manifest
├── scripts/
│   ├── send-to-hub.py       # Forwards pipeline output to Azure IoT Hub
│   └── run-edge-sim.sh      # Edge simulator launcher
├── docker-compose.yml       # Per-module containers
├── Makefile                 # build / test / run commands
├── .env.template            # Configuration template
└── GUIDE.md                 # Detailed beginner-friendly guide
```
