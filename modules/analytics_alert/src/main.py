"""
Analytics & Alert Module - Entry point.

IoT Edge mode: Receives filtered data from Edge Hub, detects anomalies, sends alerts.
Standalone mode: Reads JSON from stdin, prints alerts to stdout.
"""

import asyncio
import json
import os
import signal
import sys
from datetime import datetime, timezone

from analytics import AnalyticsEngine, AnalyticsConfig, AlertLevel

# Check if running inside IoT Edge (the runtime sets this env var)
EDGE_MODE = os.environ.get("IOTEDGE_MODULEID") is not None


def build_alert_message(alert, stats) -> str:
    """Serialize alert + stats to JSON for upstream consumption."""
    payload = {
        "alertLevel": alert.level.value,
        "message": alert.message,
        "temperature": alert.temperature,
        "threshold": alert.threshold,
        "sensorId": alert.sensor_id,
        "timestamp": alert.timestamp,
        "stats": {
            "mean": round(stats.mean, 2),
            "stdev": round(stats.stdev, 2),
            "min": round(stats.min_val, 2),
            "max": round(stats.max_val, 2),
            "trend": round(stats.trend, 2),
            "windowSize": stats.count,
        },
    }
    return json.dumps(payload)


def build_stats_message(sensor_id: str, temperature: float, stats) -> str:
    """Serialize periodic stats (even when no alert) for monitoring."""
    payload = {
        "type": "telemetry_stats",
        "sensorId": sensor_id,
        "latestTemperature": temperature,
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "stats": {
            "mean": round(stats.mean, 2),
            "stdev": round(stats.stdev, 2),
            "min": round(stats.min_val, 2),
            "max": round(stats.max_val, 2),
            "trend": round(stats.trend, 2),
            "windowSize": stats.count,
        },
    }
    return json.dumps(payload)


# ─── Standalone mode ─────────────────────────────────────────────────────────

def run_standalone():
    """Read filtered JSON from stdin, analyze, and print alerts to stdout."""
    config = AnalyticsConfig(
        alert_temp_high=float(os.environ.get("ALERT_TEMP_HIGH", "35.0")),
        alert_temp_low=float(os.environ.get("ALERT_TEMP_LOW", "-10.0")),
        rolling_window_size=int(os.environ.get("ROLLING_WINDOW_SIZE", "10")),
    )
    engine = AnalyticsEngine(config)

    print("[analytics_alert] Starting in STANDALONE mode", file=sys.stderr)
    print(f"[analytics_alert] Thresholds: low={config.alert_temp_low}, "
          f"high={config.alert_temp_high}", file=sys.stderr)
    print("---", file=sys.stderr)

    try:
        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue

            try:
                data = json.loads(line)
            except json.JSONDecodeError:
                print(f"[analytics_alert] WARNING: Invalid JSON", file=sys.stderr)
                continue

            temperature = data.get("temperature")
            sensor_id = data.get("sensorId", "unknown")

            if temperature is None:
                continue

            alert, stats = engine.process(temperature, sensor_id)

            if alert:
                alert_json = build_alert_message(alert, stats)
                # Alerts go to stdout
                print(alert_json)
                sys.stdout.flush()

                level_tag = "CRITICAL" if alert.level == AlertLevel.CRITICAL else "WARNING"
                print(f"[analytics_alert] [{level_tag}] {alert.message}",
                      file=sys.stderr)
            else:
                # Stats go to stdout for monitoring
                stats_json = build_stats_message(sensor_id, temperature, stats)
                print(stats_json)
                sys.stdout.flush()

    except KeyboardInterrupt:
        pass

    print(f"[analytics_alert] Stats: processed={engine.total_processed} "
          f"alerts={engine.total_alerts}", file=sys.stderr)
    print("[analytics_alert] Stopped.", file=sys.stderr)


# ─── IoT Edge mode ───────────────────────────────────────────────────────────

async def run_edge():
    """Run as an IoT Edge module, receiving from Edge Hub."""
    from azure.iot.device.aio import IoTHubModuleClient

    config = AnalyticsConfig(
        alert_temp_high=float(os.environ.get("ALERT_TEMP_HIGH", "35.0")),
        alert_temp_low=float(os.environ.get("ALERT_TEMP_LOW", "-10.0")),
        rolling_window_size=int(os.environ.get("ROLLING_WINDOW_SIZE", "10")),
    )
    engine = AnalyticsEngine(config)

    client = IoTHubModuleClient.create_from_edge_environment()
    await client.connect()
    print("[analytics_alert] Connected to IoT Edge Hub", file=sys.stderr)

    stop_event = asyncio.Event()

    def on_shutdown():
        stop_event.set()

    loop = asyncio.get_event_loop()
    loop.add_signal_handler(signal.SIGTERM, on_shutdown)
    loop.add_signal_handler(signal.SIGINT, on_shutdown)

    async def message_handler(message):
        try:
            data = json.loads(message.data.decode("utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError):
            print("[analytics_alert] WARNING: Could not decode message",
                  file=sys.stderr)
            return

        temperature = data.get("temperature")
        sensor_id = data.get("sensorId", "unknown")

        if temperature is None:
            return

        alert, stats = engine.process(temperature, sensor_id)

        if alert:
            alert_json = build_alert_message(alert, stats)
            # Send alert to IoT Hub (upstream)
            from azure.iot.device import Message
            msg = Message(alert_json)
            msg.content_type = "application/json"
            msg.content_encoding = "utf-8"
            msg.custom_properties["alertLevel"] = alert.level.value
            msg.custom_properties["source"] = "analyticsAlert"

            await client.send_message_to_output(msg, "alertOutput")

            level_tag = "CRITICAL" if alert.level == AlertLevel.CRITICAL else "WARNING"
            print(f"[analytics_alert] [{level_tag}] {alert.message}",
                  file=sys.stderr)

    client.on_message_received = message_handler

    await stop_event.wait()
    await client.shutdown()

    print(f"[analytics_alert] Stats: processed={engine.total_processed} "
          f"alerts={engine.total_alerts}", file=sys.stderr)
    print("[analytics_alert] Stopped.", file=sys.stderr)


# ─── Entry point ──────────────────────────────────────────────────────────────

if __name__ == "__main__":
    if EDGE_MODE:
        asyncio.run(run_edge())
    else:
        run_standalone()
