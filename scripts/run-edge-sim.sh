#!/usr/bin/env bash
# ─── Run pipeline using iotedgehubdev simulator ───
# Prerequisites:
#   pip install iotedgehubdev
#   az iot hub device-identity create --hub-name <hub> --device-id <device> --edge-enabled
#
# This script:
# 1. Builds all Docker images
# 2. Generates a deployment manifest from the template
# 3. Starts the IoT Edge Hub simulator
# 4. Deploys all modules

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

# Load environment
if [ -f "$PROJECT_DIR/.env" ]; then
    set -a
    source "$PROJECT_DIR/.env"
    set +a
else
    echo "ERROR: .env file not found. Run 'make setup' first."
    exit 1
fi

echo "=== Building Docker images ==="
cd "$PROJECT_DIR"
make docker

echo ""
echo "=== Generating deployment manifest ==="

# Substitute env vars into deployment template
MANIFEST_TEMPLATE="$PROJECT_DIR/config/deployment.template.json"
MANIFEST_OUTPUT="$PROJECT_DIR/config/deployment.generated.json"

envsubst < "$MANIFEST_TEMPLATE" > "$MANIFEST_OUTPUT"
echo "Generated: $MANIFEST_OUTPUT"

echo ""
echo "=== Setting up iotedgehubdev ==="

# Setup the simulator with your IoT Hub connection
iotedgehubdev setup -c "$EDGE_DEVICE_CONNECTION_STRING"

echo ""
echo "=== Starting IoT Edge simulator ==="

# Start with the generated manifest
iotedgehubdev start -d "$MANIFEST_OUTPUT"

echo ""
echo "=== Pipeline is running ==="
echo "Monitor with: docker logs -f sensor-simulator"
echo "              docker logs -f data-filter"
echo "              docker logs -f analytics-alert"
echo ""
echo "Stop with: iotedgehubdev stop"
