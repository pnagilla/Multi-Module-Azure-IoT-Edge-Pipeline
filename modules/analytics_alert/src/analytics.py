"""Analytics engine for temperature data with threshold breach detection."""

import statistics
from collections import deque
from dataclasses import dataclass, field
from datetime import datetime, timezone
from enum import Enum
from typing import Optional


class AlertLevel(Enum):
    NONE = "none"
    WARNING = "warning"
    CRITICAL = "critical"


@dataclass
class Alert:
    level: AlertLevel
    message: str
    temperature: float
    threshold: float
    sensor_id: str
    timestamp: str


@dataclass
class AnalyticsConfig:
    alert_temp_high: float = 35.0
    alert_temp_low: float = -10.0
    warning_margin: float = 5.0       # degrees before threshold triggers warning
    rolling_window_size: int = 10
    trend_sensitivity: float = 0.5    # degrees per reading for trend alert


@dataclass
class Stats:
    """Rolling statistics for the analytics window."""
    mean: float = 0.0
    stdev: float = 0.0
    min_val: float = 0.0
    max_val: float = 0.0
    trend: float = 0.0  # positive = rising, negative = falling
    count: int = 0


class AnalyticsEngine:
    """Processes filtered sensor data, computes rolling stats, and detects threshold breaches."""

    def __init__(self, config: Optional[AnalyticsConfig] = None):
        self.config = config or AnalyticsConfig()
        self._window: deque[float] = deque(maxlen=self.config.rolling_window_size)
        self._total_processed: int = 0
        self._total_alerts: int = 0

    def process(self, temperature: float, sensor_id: str) -> tuple[Optional[Alert], Stats]:
        """Process a temperature reading. Returns an alert (if any) and current stats."""
        self._window.append(temperature)
        self._total_processed += 1

        stats = self._compute_stats()
        alert = self._check_thresholds(temperature, sensor_id, stats)

        if alert:
            self._total_alerts += 1

        return alert, stats

    def _compute_stats(self) -> Stats:
        if not self._window:
            return Stats()

        values = list(self._window)
        s = Stats()
        s.count = len(values)
        s.mean = statistics.mean(values)
        s.stdev = statistics.stdev(values) if len(values) >= 2 else 0.0
        s.min_val = min(values)
        s.max_val = max(values)

        # Simple linear trend: difference between recent and older halves
        if len(values) >= 4:
            mid = len(values) // 2
            older_avg = statistics.mean(values[:mid])
            newer_avg = statistics.mean(values[mid:])
            s.trend = newer_avg - older_avg
        else:
            s.trend = 0.0

        return s

    def _check_thresholds(self, temperature: float, sensor_id: str,
                          stats: Stats) -> Optional[Alert]:
        now = datetime.now(timezone.utc).isoformat()

        # Critical: temperature exceeds hard thresholds
        if temperature >= self.config.alert_temp_high:
            return Alert(
                level=AlertLevel.CRITICAL,
                message=f"Temperature {temperature:.1f}C exceeds high threshold "
                        f"{self.config.alert_temp_high:.1f}C",
                temperature=temperature,
                threshold=self.config.alert_temp_high,
                sensor_id=sensor_id,
                timestamp=now,
            )

        if temperature <= self.config.alert_temp_low:
            return Alert(
                level=AlertLevel.CRITICAL,
                message=f"Temperature {temperature:.1f}C below low threshold "
                        f"{self.config.alert_temp_low:.1f}C",
                temperature=temperature,
                threshold=self.config.alert_temp_low,
                sensor_id=sensor_id,
                timestamp=now,
            )

        # Warning: approaching thresholds
        high_warning = self.config.alert_temp_high - self.config.warning_margin
        low_warning = self.config.alert_temp_low + self.config.warning_margin

        if temperature >= high_warning:
            return Alert(
                level=AlertLevel.WARNING,
                message=f"Temperature {temperature:.1f}C approaching high threshold "
                        f"(warning at {high_warning:.1f}C)",
                temperature=temperature,
                threshold=high_warning,
                sensor_id=sensor_id,
                timestamp=now,
            )

        if temperature <= low_warning:
            return Alert(
                level=AlertLevel.WARNING,
                message=f"Temperature {temperature:.1f}C approaching low threshold "
                        f"(warning at {low_warning:.1f}C)",
                temperature=temperature,
                threshold=low_warning,
                sensor_id=sensor_id,
                timestamp=now,
            )

        # Warning: rapid trend
        if abs(stats.trend) >= self.config.trend_sensitivity and stats.count >= 4:
            direction = "rising" if stats.trend > 0 else "falling"
            return Alert(
                level=AlertLevel.WARNING,
                message=f"Rapid temperature {direction}: trend={stats.trend:+.2f}C "
                        f"over last {stats.count} readings",
                temperature=temperature,
                threshold=self.config.trend_sensitivity,
                sensor_id=sensor_id,
                timestamp=now,
            )

        return None

    @property
    def total_processed(self) -> int:
        return self._total_processed

    @property
    def total_alerts(self) -> int:
        return self._total_alerts
