"""
Direct-to-Hub sender: Runs the local pipeline and forwards output to Azure IoT Hub.

This bypasses iotedgehubdev entirely. It:
1. Reads pipeline output from stdin (pipe from make run-local)
2. Sends each message to IoT Hub using the device connection string

Usage:
    make run-local | python3 scripts/send-to-hub.py

Requires: pip install azure-iot-device (in a venv)
"""

import json
import os
import sys
import time

def main():
    conn_str = os.environ.get("EDGE_DEVICE_CONNECTION_STRING", "")
    if not conn_str:
        print("[send-to-hub] ERROR: EDGE_DEVICE_CONNECTION_STRING not set", file=sys.stderr)
        print("[send-to-hub] Run: source .env", file=sys.stderr)
        sys.exit(1)

    # Import here so the error message above shows before any import errors
    from azure.iot.device import IoTHubDeviceClient, Message

    print("[send-to-hub] Connecting to IoT Hub...", file=sys.stderr)
    client = IoTHubDeviceClient.create_from_connection_string(conn_str)
    client.connect()
    print("[send-to-hub] Connected! Forwarding pipeline output to IoT Hub.", file=sys.stderr)
    print("[send-to-hub] Press Ctrl+C to stop.", file=sys.stderr)

    sent = 0
    try:
        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue

            try:
                data = json.loads(line)
            except json.JSONDecodeError:
                continue

            msg = Message(line)
            msg.content_type = "application/json"
            msg.content_encoding = "utf-8"

            # Tag the message type for IoT Hub routing
            if "alertLevel" in data:
                msg.custom_properties["messageType"] = "alert"
                msg.custom_properties["alertLevel"] = data.get("alertLevel", "unknown")
            else:
                msg.custom_properties["messageType"] = "telemetry"

            client.send_message(msg)
            sent += 1

            msg_type = data.get("alertLevel", data.get("type", "unknown"))
            print(f"[send-to-hub] Sent #{sent}: {msg_type}", file=sys.stderr)

    except KeyboardInterrupt:
        pass
    finally:
        client.disconnect()
        print(f"\n[send-to-hub] Done. Sent {sent} messages to IoT Hub.", file=sys.stderr)


if __name__ == "__main__":
    main()
