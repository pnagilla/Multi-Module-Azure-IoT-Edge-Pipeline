# Multi-Module Azure IoT Edge Pipeline - Complete Guide

A beginner-friendly, interview-ready reference for understanding this project and Azure IoT Edge.

---

## Table of Contents

1. [The Real World Problem](#1-the-real-world-problem)
2. [What is Edge Computing](#2-what-is-edge-computing)
3. [Azure IoT - The Microsoft Solution](#3-azure-iot---the-microsoft-solution)
4. [What is Docker](#4-what-is-docker)
5. [Our Project - Step by Step](#5-our-project---step-by-step)
6. [How Messages Flow (Routing)](#6-how-messages-flow-routing)
7. [The Deployment Manifest](#7-the-deployment-manifest)
8. [Standalone vs Edge Mode](#8-standalone-vs-edge-mode)
9. [Docker and Containerization](#9-docker-and-containerization)
10. [Project Structure](#10-project-structure)
11. [Build and Run Commands](#11-build-and-run-commands)
12. [Azure Setup Steps](#12-azure-setup-steps)
13. [Common Interview Questions](#13-common-interview-questions)
14. [Where IoT Edge is Used](#14-where-iot-edge-is-used)
15. [Key Terms Cheat Sheet](#15-key-terms-cheat-sheet)
16. [Our Project vs Production](#16-our-project-vs-production)

---

## 1. The Real World Problem

### What is a sensor?

A sensor is a tiny electronic device that measures something physical - temperature, humidity, pressure, motion, light, etc.

Real-world examples you already know:
- The thermometer in your AC unit - it's a temperature sensor
- Your phone's accelerometer - detects if you tilt it
- A motion detector that turns on hallway lights
- A smoke detector in your kitchen

A sensor does ONE thing: it reads a value. A temperature sensor might say "it's 22.5 degrees right now." That's it. It can't think, analyze, or make decisions.

### What is IoT (Internet of Things)?

IoT means connecting these sensors to the internet so you can:
- See the data remotely (check your home temperature from your phone)
- Get alerts (smoke detected! temperature too high!)
- Analyze trends (your factory was 2 degrees warmer this month)

Simple example: A farmer has 50 greenhouses. Each has temperature and humidity sensors. Without IoT, he has to walk to each greenhouse to check. With IoT, all sensors send data to the cloud, and he sees everything on his phone.

### The problem with sending everything to the cloud

Imagine a factory with 10,000 sensors, each generating a reading every second:
- That's 10,000 messages per second
- 864 million messages per day
- Huge internet bandwidth cost
- If internet goes down for 1 hour, you lose 36 million readings
- A critical alert (machine overheating!) has to travel to the cloud and back - that's 100-500ms delay. In some cases, that delay can cause damage

---

## 2. What is Edge Computing

### The analogy

Think of it like a company with a head office and branch offices:

**Without edge computing** (everything goes to head office):
```
Branch Office (sensor) -> sends ALL paperwork to Head Office (cloud)
                       -> Head Office reviews it
                       -> Head Office sends instructions back
```
Problem: Slow, expensive, and if the phone line is cut, work stops.

**With edge computing** (smart branch manager):
```
Branch Office (sensor) -> Branch Manager (edge device) reviews locally
                       -> Handles routine stuff immediately
                       -> Only sends important reports to Head Office
```
The Branch Manager is the IoT Edge device.

### What is an Edge Device?

It's a small computer sitting physically close to the sensors. It could be:
- A Raspberry Pi (small $35 computer)
- An industrial PC in a factory
- A gateway box mounted on a wall
- Even your laptop (which is what we used!)

This computer runs your code to process sensor data locally, without needing the cloud for every decision.

---

## 3. Azure IoT - The Microsoft Solution

Microsoft provides three main pieces:

### Piece 1: Azure IoT Hub (lives in the cloud)

Think of it as the head office reception desk. It:
- Knows which devices exist (device registry)
- Receives important messages from devices
- Can send commands back to devices
- Keeps track of device health

In our project: `iot-edge-hub-prashanth-01` - this is the reception desk we created.

### Piece 2: Azure IoT Edge Runtime (lives on the device)

This is the operating system for your edge device. It has two parts:

**Edge Agent** - the IT manager on the device:
- Receives instructions from IoT Hub: "install this module," "update that module"
- Downloads Docker containers (modules)
- Starts, stops, and monitors modules
- Reports health back to IoT Hub: "all modules running OK"

**Edge Hub** - the internal mail room on the device:
- Routes messages between modules
- Decides what goes to the cloud and what stays local
- Stores messages when internet is down (store-and-forward)
- It's a local mini-version of IoT Hub

### Piece 3: Modules (your code)

Modules are Docker containers - little packages of your code that run independently. Think of them as employees in the branch office, each with a specific job.

---

## 4. What is Docker

Imagine you write a program on your Mac. You want to run it on a Raspberry Pi in a factory. Problem: different operating system, different libraries, different everything.

Docker solves this by putting your program + everything it needs into a container - a self-contained package that runs the same way everywhere.

Think of it as shipping containers on cargo ships:
- Every container is the same size/shape (standardized)
- Doesn't matter what's inside
- Can be loaded on any ship (any computer)
- Contents are isolated from each other

In our project:
- `sensor_simulator` container: has the C++ binary + Ubuntu libraries
- `data_filter` container: has the C++ binary + Ubuntu libraries
- `analytics_alert` container: has Python + our analytics code

---

## 5. Our Project - Step by Step

### The scenario

You work at a cold storage facility (warehouse that keeps food frozen). You need to monitor temperature. If it gets too warm, food spoils - that's expensive.

### What we built

Three "employees" (modules) working together:

```
Employee 1          Employee 2           Employee 3
SENSOR READER  ->   QUALITY CHECK   ->   ANALYST
"The temperature     "Is this reading     "Should we
 is 22.5C"           valid or garbage?"    sound the alarm?"
                                                |
                                                v
                                          HEAD OFFICE
                                          (Azure IoT Hub)
                                          "Alert received!"
```

### Architecture Diagram

```
+------------------- IoT Edge Device (your Mac) -------------------+
|                                                                   |
|  +--------------+    +--------------+    +------------------+    |
|  |   Sensor      |    |   Data       |    |   Analytics      |    |
|  |   Simulator   |--->|   Filter     |--->|   & Alert        |    |
|  |   (C++)       |    |   (C++)      |    |   (Python)       |    |
|  +--------------+    +--------------+    +--------+---------+    |
|         |                    |                     |              |
|         |         +----------v----------+          |              |
|         +-------->|    Edge Hub          |<---------+              |
|                   |  (message broker)    |                        |
|                   +----------+----------+                        |
|                              |                                   |
+------------------------------+-----------------------------------+
                               | only alerts go upstream
                               v
                    +------------------+
                    |  Azure IoT Hub    |
                    |  (cloud)          |
                    +------------------+
```

### Module 1: Sensor Simulator (C++)

**Real job**: Read the thermometer every 3 seconds

Since we don't have a real thermometer connected to our Mac, we simulate one. Our code generates realistic temperature readings with:
- A base temperature around 22C
- Random noise (+/-2C) - real sensors aren't perfectly accurate
- Gradual drift - temperature naturally rises and falls over time

**What it produces** (every 3 seconds):
```json
{
  "sensorId": "temp-sensor-001",
  "temperature": 22.53,
  "humidity": 45.2,
  "timestamp": "2026-02-15T20:36:55Z",
  "sequenceNumber": 42
}
```

**Why C++?** In real hardware, the code that talks to sensors must be fast and efficient. C++ gives you direct hardware access and minimal memory usage - critical when running on a tiny device with 256MB RAM.

**Key files**:
- `modules/sensor_simulator/src/main.cpp` - entry point, standalone and Edge modes
- `modules/sensor_simulator/src/sensor.cpp` - temperature simulation with Brownian-motion drift
- `modules/sensor_simulator/src/message_builder.cpp` - JSON serialization
- `modules/sensor_simulator/include/sensor.h` - sensor class definition

### Module 2: Data Filter (C++)

**Real job**: Check if the reading makes sense before passing it on

Why do we need this? Real sensors produce garbage data sometimes:
- A loose wire might read -999C (impossible)
- Electrical interference might cause a spike from 22C to 80C in one second
- A dying sensor might output random values

**Our filter does two checks**:

**Check 1 - Range validation**: Is the reading physically possible?
```
If temperature < -40C -> REJECT (sensor can't measure below this)
If temperature > 85C  -> REJECT (sensor can't measure above this)
```

**Check 2 - Spike detection**: Did the value jump too fast?
```
Recent readings: 22.0, 22.1, 22.3, 22.2
New reading: 55.0 -> REJECT (jumped 33 degrees in 3 seconds - impossible)
```

It uses statistics to detect spikes: it keeps a window of recent readings, calculates the average and standard deviation, and rejects outliers.

**What comes in vs what goes out**:
```
IN:  347 readings from sensor
OUT: 316 clean readings (31 rejected as noise/spikes)
```

**Key files**:
- `modules/data_filter/src/main.cpp` - entry point
- `modules/data_filter/src/filter.cpp` - range validation and spike detection logic
- `modules/data_filter/src/json_parser.cpp` - JSON parsing without external dependencies
- `modules/data_filter/include/filter.h` - filter configuration and class definition

### Module 3: Analytics & Alert (Python)

**Real job**: Analyze the clean data and decide if we need to alert someone

This is the "brain." It computes:

**Rolling statistics** (over the last 10 readings):
- Mean (average): "the average temperature is 22.3C"
- Standard deviation: "readings are varying by +/-1.5C"
- Trend: "temperature is rising at 0.5C per reading cycle"

**Alert logic**:
```
Temperature > 35C     -> CRITICAL ALERT  (food is spoiling!)
Temperature < -10C    -> CRITICAL ALERT  (equipment failure!)
Temperature > 30C     -> WARNING         (approaching danger zone)
Temperature < -5C     -> WARNING         (approaching danger zone)
Trend > 0.5C/reading  -> WARNING         (temperature rising fast)
```

**Why Python?** Analytics, statistics, and machine learning libraries are best in Python. In production, you might use pandas, NumPy, or TensorFlow here.

**Key files**:
- `modules/analytics_alert/src/main.py` - entry point, standalone and Edge modes
- `modules/analytics_alert/src/analytics.py` - analytics engine with rolling stats and alert detection
- `modules/analytics_alert/tests/test_analytics.py` - unit tests

---

## 6. How Messages Flow (Routing)

This is the most important concept for interviews.

### The Route Map

Imagine a post office with sorting rules:

```
Rule 1: "All letters FROM Sensor Simulator,
         deliver TO Data Filter's mailbox"

Rule 2: "All letters FROM Data Filter,
         deliver TO Analytics Alert's mailbox"

Rule 3: "All letters FROM Analytics Alert,
         deliver TO the Head Office (cloud)"
```

In our `config/deployment.template.json`, these rules look like:

```
sensorToFilter:
  FROM sensorSimulator/outputs/sensorOutput
  INTO dataFilter/inputs/filterInput

filterToAnalytics:
  FROM dataFilter/outputs/filterOutput
  INTO analyticsAlert/inputs/analyticsInput

alertsToHub:
  FROM analyticsAlert/outputs/alertOutput
  INTO $upstream    <-- this means "send to cloud"
```

**Key concepts**:
- `$upstream` = send to IoT Hub in the cloud
- `BrokeredEndpoint("/modules/X/inputs/Y")` = send to another module's input
- Routes can include conditions: e.g., only route messages where alertLevel = 'critical'
- Priority: lower number = higher priority
- TTL (Time to Live): how long Edge Hub stores messages if the next hop is unavailable (we set 7200 seconds = 2 hours)

**Key insight**: Modules never talk to each other directly. All messages go through Edge Hub (the post office). This means you can:
- Add a new module without changing existing ones
- Remove a module without breaking others
- Change routing without redeploying code

### What actually goes to the cloud?

Out of 347 sensor readings:
- 31 rejected by filter (never leave the device)
- ~300 pass through as telemetry stats (sent to cloud)
- ~16 trigger alerts (sent to cloud with priority)

Without edge computing, all 347 raw readings would go to the cloud. With edge computing, we process locally and only send meaningful data.

---

## 7. The Deployment Manifest

The deployment manifest (`config/deployment.template.json`) is like a job order you send to the device:

```
"Dear Edge Device,

Please run these 3 containers:
1. sensor-simulator (image: localhost:5000/sensor-simulator:1.0.0)
   - Set TELEMETRY_INTERVAL_MS to 3000
   - Restart it if it crashes

2. data-filter (image: localhost:5000/data-filter:1.0.0)
   - Set TEMP_MAX_VALID to 85.0
   - Restart it if it crashes

3. analytics-alert (image: localhost:5000/analytics-alert:1.0.0)
   - Set ALERT_TEMP_HIGH to 35.0
   - Restart it if it crashes

And route messages like this:
   sensor -> filter -> analytics -> cloud

Thanks,
IoT Hub"
```

In production, you push this manifest to IoT Hub, and the Edge Agent on the device automatically applies it. You never SSH into the device.

The manifest defines:
1. **System modules**: Edge Agent and Edge Hub versions and config
2. **Custom modules**: our 3 modules - container image, environment variables, restart policy
3. **Routes**: how messages flow between modules
4. **Store and forward config**: offline buffering behavior

---

## 8. Standalone vs Edge Mode

Our modules compile in two modes:

| Mode | How it works | When to use |
|------|-------------|-------------|
| **Standalone** | Reads stdin, writes stdout, no Azure SDK | Local development, testing, CI/CD |
| **IoT Edge** | Uses Azure IoT SDK, Edge Hub for messaging | Production deployment |

This is controlled by the `STANDALONE_MODE` CMake flag. The same codebase, two build targets.

**Why this is a best practice**: It lets you:
- Develop and test without Azure (zero cost)
- Run unit tests in CI/CD pipelines
- Debug locally with simple Unix pipes: `sensor | filter | analytics`
- Deploy the same code to production with just a build flag change

---

## 9. Docker and Containerization

Each module has a multi-stage Dockerfile:

```
Stage 1 (builder): Ubuntu + build tools -> compile code
Stage 2 (runtime): Ubuntu minimal + just the binary
```

**Why multi-stage**:
- Build image: ~500MB (cmake, gcc, headers)
- Runtime image: ~50MB (just the binary + libs)
- Smaller images = faster deployment to edge devices

**Security best practices we followed**:
- `USER iotedge` - run as non-root
- `--no-install-recommends` - minimal package installs
- `.dockerignore` - prevent local build artifacts from leaking into images

---

## 10. Project Structure

```
Multi-Module-Azure-IoT-Edge-Pipeline/
+-- config/
|   +-- deployment.template.json    # IoT Edge deployment manifest
+-- modules/
|   +-- sensor_simulator/           # Module 1: C++ temperature sensor
|   |   +-- include/
|   |   |   +-- sensor.h
|   |   |   +-- message_builder.h
|   |   +-- src/
|   |   |   +-- main.cpp
|   |   |   +-- sensor.cpp
|   |   |   +-- message_builder.cpp
|   |   +-- CMakeLists.txt
|   |   +-- Dockerfile
|   |   +-- .dockerignore
|   +-- data_filter/                # Module 2: C++ data validator
|   |   +-- include/
|   |   |   +-- filter.h
|   |   |   +-- json_parser.h
|   |   +-- src/
|   |   |   +-- main.cpp
|   |   |   +-- filter.cpp
|   |   |   +-- json_parser.cpp
|   |   +-- CMakeLists.txt
|   |   +-- Dockerfile
|   |   +-- .dockerignore
|   +-- analytics_alert/            # Module 3: Python analytics
|       +-- src/
|       |   +-- main.py
|       |   +-- analytics.py
|       +-- tests/
|       |   +-- test_analytics.py
|       +-- requirements.txt
|       +-- Dockerfile
|       +-- .dockerignore
+-- scripts/
|   +-- run-edge-sim.sh             # iotedgehubdev launcher
|   +-- send-to-hub.py              # Direct-to-hub forwarder
+-- docker-compose.yml              # Per-module containers
+-- docker-compose.pipeline.yml     # Named-pipe pipeline
+-- Makefile                        # Build/test/run commands
+-- .env.template                   # Configuration template
+-- .gitignore
```

---

## 11. Build and Run Commands

### Prerequisites

| Tool | Install |
|------|---------|
| CMake 3.14+ | `brew install cmake` |
| C++17 compiler | Xcode CLT (`xcode-select --install`) |
| Python 3.11+ | `brew install python` |
| Docker Desktop | docker.com |
| Azure CLI (optional) | `brew install azure-cli` |

### Commands

```bash
# Setup
make setup              # Verify prerequisites, create .env

# Build
make build              # Build both C++ modules locally (standalone mode)
make build-sensor       # Build sensor_simulator only
make build-filter       # Build data_filter only

# Test
make test               # Run all tests
make test-analytics     # Run Python analytics unit tests

# Run locally (no Azure needed)
make run-local          # Full pipeline: sensor | filter | analytics
make run-sensor         # Sensor only

# Docker
make docker             # Build all Docker images
make run-pipeline       # Run pipeline via Docker Compose

# Run with Azure IoT Hub
python3 -m venv .venv
source .venv/bin/activate
pip install azure-iot-device
set -a && source .env && set +a && make run-local | python3 scripts/send-to-hub.py

# Monitor events in another terminal
az iot hub monitor-events --hub-name <your-hub-name>

# Clean up
make clean              # Remove build artifacts
```

---

## 12. Azure Setup Steps

### Step 1: Create a Free Azure Account
- Go to https://azure.microsoft.com/free
- Sign up (needs phone number + credit card for verification, no charges)
- Get $200 free credit for 30 days + free-tier services for 12 months

### Step 2: Install Azure CLI and Login
```bash
brew install azure-cli
az login
```

### Step 3: Register IoT Provider
```bash
az provider register --namespace Microsoft.Devices
# Wait until it shows "Registered":
az provider show --namespace Microsoft.Devices --query "registrationState" --output tsv
```

### Step 4: Create Resources
```bash
# Resource group (folder for your resources)
az group create --name iot-edge-rg --location eastus

# IoT Hub (free tier)
az iot hub create \
  --name iot-edge-hub-<your-name> \
  --resource-group iot-edge-rg \
  --sku F1 \
  --partition-count 2

# Install IoT extension
az extension add --name azure-iot

# Register an Edge device
az iot hub device-identity create \
  --hub-name iot-edge-hub-<your-name> \
  --device-id edge-dev-01 \
  --edge-enabled

# Get connection string
az iot hub device-identity connection-string show \
  --hub-name iot-edge-hub-<your-name> \
  --device-id edge-dev-01 \
  --output tsv
```

### Step 5: Configure and Run
```bash
make setup
# Edit .env - paste connection string (in quotes!) into EDGE_DEVICE_CONNECTION_STRING

# Run the pipeline with IoT Hub
source .venv/bin/activate
set -a && source .env && set +a && make run-local | python3 scripts/send-to-hub.py
```

### Step 6: Monitor
```bash
az iot hub monitor-events --hub-name iot-edge-hub-<your-name>
```

### Step 7: Check Azure Resources
- Go to https://portal.azure.com
- Search for "All resources" - see everything in your subscription
- Search for "iot-edge-rg" - see the IoT Hub inside it
- Click the IoT Hub -> "IoT Edge" to see your device

### Cleanup (when done)
```bash
az group delete --name iot-edge-rg --yes --no-wait
```

### Cost Summary

| Resource | Tier | Cost |
|----------|------|------|
| Azure Account | Free trial | $0 ($200 credit) |
| IoT Hub | F1 (free) | $0 (8K msgs/day, forever) |
| Edge Simulator | Local Docker | $0 |

---

## 13. Common Interview Questions

**Q: What is Azure IoT Edge?**
> IoT Edge is a runtime that runs on devices and lets you deploy cloud workloads locally as Docker containers called modules. Instead of sending all raw data to the cloud, you process it at the edge (close to the source) and only send what matters.

**Q: What is the difference between IoT Hub and IoT Edge?**
> IoT Hub is the cloud service that manages devices and receives messages. IoT Edge is the runtime that runs on the device itself, enabling local processing via Docker containers. Edge Hub (running locally) acts as a local proxy for IoT Hub.

**Q: Why use IoT Edge instead of sending everything to the cloud?**
> Three reasons: (1) Latency - local processing is faster for real-time decisions. (2) Bandwidth - filter data locally, send only what matters. (3) Offline resilience - Edge Hub buffers messages when internet is down.

**Q: How do modules communicate?**
> Never directly. All communication goes through Edge Hub using a publish/subscribe pattern. Routes in the deployment manifest define the message flow. This decouples modules - you can add, remove, or replace modules without changing others.

**Q: What happens when the device loses internet?**
> Edge Hub has store-and-forward. Messages destined for $upstream are queued locally (configurable TTL, we set 2 hours). When connectivity returns, they're automatically forwarded to IoT Hub in order.

**Q: How do you deploy updates to edge devices?**
> Update the deployment manifest in IoT Hub. The Edge Agent on each device periodically checks for changes, pulls new container images, and restarts modules. No SSH required. You can target individual devices or groups using automatic deployments.

**Q: What's the difference between an IoT Edge module and a regular Docker container?**
> An IoT Edge module is a Docker container that uses the Azure IoT SDK to communicate with Edge Hub. It has defined inputs/outputs, can receive module twin updates (configuration changes from the cloud), and reports its status to IoT Hub via the Edge Agent.

**Q: What is a Module Twin?**
> A JSON document stored in IoT Hub that contains desired and reported properties for a module. You can update the desired properties from the cloud, and the module receives the update and applies the new configuration without redeploying. Example: changing the alert threshold from 35C to 40C without rebuilding the container.

**Q: What protocols does IoT Edge support?**
> MQTT (default, lightweight), AMQP (enterprise messaging), and HTTPS. Edge Hub supports all three. Our modules use MQTT via MQTT_Protocol in the C++ code.

**Q: What is a deployment manifest?**
> A JSON file that defines what modules to run on an Edge device, their container images, environment variables, restart policies, and message routing rules. You push it to IoT Hub and the Edge Agent automatically applies it.

**Q: How do you monitor IoT Edge devices?**
> Multiple ways: (1) `az iot hub monitor-events` for real-time message monitoring. (2) IoT Hub metrics in Azure Portal for message counts and device health. (3) Azure Monitor for logs and alerts. (4) Device twins for reported properties and status.

### How to Talk About This in Your Interview

> "I built a multi-module IoT Edge pipeline with three modules - a C++ sensor simulator, a C++ data filter for noise rejection and spike detection, and a Python analytics engine for threshold alerting. The modules communicate through Edge Hub routing, with only alerts forwarded upstream to IoT Hub. I used multi-stage Docker builds for minimal image sizes, implemented dual-mode compilation for local development without Azure dependencies, and deployed to Azure IoT Hub's free tier for end-to-end validation. The pipeline demonstrated ~9% data reduction through edge filtering before any data reached the cloud."

---

## 14. Where IoT Edge is Used

### Industrial / Manufacturing (most common)
- Factory machines with temperature, pressure, vibration sensors
- Predictive maintenance - detect when a motor is about to fail
- Quality control - camera + AI module inspecting products on an assembly line

### Energy and Utilities
- Oil pipeline monitoring (pressure, flow rate, leak detection)
- Wind turbines reporting performance data
- Smart grid - managing power distribution locally
- Often in remote locations with poor internet - edge computing is essential

### Retail
- Smart checkout systems (cameras + AI detecting products)
- Refrigerator temperature monitoring in supermarkets
- Foot traffic analytics using in-store cameras

### Healthcare
- Patient monitoring devices in hospitals
- Medical imaging - running AI models on X-rays locally (data privacy regulations may prevent sending patient data to the cloud)

### Transportation
- Connected vehicles sending telemetry
- Fleet tracking (GPS, fuel, driver behavior)
- Railway track monitoring

### Video/AI at the Edge
- Security cameras running object detection locally
- Counting people entering a building
- License plate recognition at parking garages

### Why not just use a regular server?

| Feature | Regular Docker on a server | IoT Edge |
|---------|---------------------------|----------|
| Remote deployment | You SSH and deploy manually | Push manifest from cloud, auto-deploys |
| Manage 10,000 devices | Nightmare | Built-in device groups and auto-deployment |
| Offline operation | You build it yourself | Store-and-forward built in |
| Module routing | You wire it yourself | Declarative routes in JSON |
| Security | You manage certificates | Automatic certificate rotation |
| Monitoring | You set it up | IoT Hub tracks device health automatically |

---

## 15. Key Terms Cheat Sheet

| Term | Simple Meaning |
|------|---------------|
| **IoT** | Connecting physical devices to the internet |
| **Sensor** | Hardware that measures something (temp, humidity, etc.) |
| **Edge Computing** | Processing data close to where it's generated, not in the cloud |
| **IoT Hub** | Microsoft's cloud service for managing IoT devices |
| **IoT Edge** | Runtime that runs on the device to process data locally |
| **Edge Agent** | Downloads and manages containers on the device |
| **Edge Hub** | Local message broker that routes data between modules |
| **Module** | A Docker container running on an Edge device |
| **Route** | A rule that tells Edge Hub where to send a message |
| **$upstream** | Special keyword meaning "send to the cloud (IoT Hub)" |
| **D2C** | Device-to-Cloud messages (sensor data going up) |
| **C2D** | Cloud-to-Device messages (commands going down) |
| **Module Twin** | Cloud-managed configuration for a module (change settings without redeploying) |
| **Store and Forward** | Edge Hub saves messages locally when internet is down |
| **Deployment Manifest** | JSON file that defines what modules to run and how to route messages |
| **MQTT** | Lightweight messaging protocol designed for IoT (low bandwidth, reliable) |
| **Telemetry** | Data flowing from device to cloud (sensor readings) |
| **OPC-UA / Modbus** | Industrial protocols for reading data from factory machines |
| **Resource Group** | A folder in Azure that groups related resources together |
| **Connection String** | A credentials string that lets your device authenticate with IoT Hub |
| **Container Registry** | A storage service for Docker images (like Docker Hub or Azure Container Registry) |
| **Multi-stage Build** | Docker technique: build in one stage, copy only the binary to a minimal runtime stage |
| **SKU** | Stock Keeping Unit - Azure's way of saying "pricing tier" (F1 = Free, S1 = Standard) |

---

## 16. Our Project vs Production

| Aspect | Our Project | Production |
|--------|------------|------------|
| Sensor | Simulated (random) | Real hardware (Modbus, OPC-UA) |
| Edge Device | Mac (local) | Raspberry Pi, industrial gateway |
| Container Registry | localhost:5000 | Azure Container Registry (ACR) |
| Deployment | Manual script | IoT Hub automatic deployments |
| Monitoring | CLI (`az iot hub monitor-events`) | Azure Monitor, Time Series Insights |
| Storage | None | Azure Blob Storage, Cosmos DB |
| Scale | 1 device | Thousands of devices via deployment groups |
| Communication | Unix pipes (standalone) | Edge Hub MQTT routing |
| Security | Local connection string | X.509 certificates, HSM |

The architecture and code patterns are identical - that's the point of this project. The same modules that run on your Mac with `make run-local` can be deployed to a real edge device with zero code changes.

---

## Configuration Reference

All behavior is tunable via environment variables (see `.env.template`):

| Variable | Default | Description |
|----------|---------|-------------|
| `TELEMETRY_INTERVAL_MS` | 3000 | How often sensor generates readings |
| `SENSOR_ID` | temp-sensor-001 | Identifier for the sensor |
| `TEMP_MIN_VALID` / `TEMP_MAX_VALID` | -40 / 85 | Physical sensor range for filtering |
| `NOISE_THRESHOLD` | 0.5 | Spike detection sensitivity |
| `ALERT_TEMP_HIGH` / `ALERT_TEMP_LOW` | 35 / -10 | Alert thresholds |
| `ROLLING_WINDOW_SIZE` | 10 | Number of readings for stats window |

## Production Readiness Features

- **Dual-mode binaries**: Every module compiles in standalone (local dev) or IoT Edge mode (production) via STANDALONE_MODE flag
- **Non-root containers**: All Dockerfiles run as `iotedge` user
- **Multi-stage Docker builds**: Minimal runtime images without build tools
- **Graceful shutdown**: Signal handlers (SIGINT/SIGTERM) in all modules
- **Store-and-forward**: Edge Hub configured with 2-hour TTL for offline resilience
- **Structured logging**: All status to stderr, data to stdout
- **Unit tests**: Analytics engine has test coverage for all alert paths
