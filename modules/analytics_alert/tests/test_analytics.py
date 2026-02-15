"""Unit tests for the analytics engine."""

import sys
import os

# Add src to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "src"))

from analytics import AnalyticsEngine, AnalyticsConfig, AlertLevel


def test_normal_reading_no_alert():
    engine = AnalyticsEngine(AnalyticsConfig(alert_temp_high=35.0, alert_temp_low=-10.0))
    alert, stats = engine.process(22.0, "sensor-1")
    assert alert is None
    assert stats.count == 1


def test_high_threshold_critical():
    engine = AnalyticsEngine(AnalyticsConfig(alert_temp_high=35.0))
    alert, _ = engine.process(36.0, "sensor-1")
    assert alert is not None
    assert alert.level == AlertLevel.CRITICAL
    assert "exceeds high threshold" in alert.message


def test_low_threshold_critical():
    engine = AnalyticsEngine(AnalyticsConfig(alert_temp_low=-10.0))
    alert, _ = engine.process(-15.0, "sensor-1")
    assert alert is not None
    assert alert.level == AlertLevel.CRITICAL
    assert "below low threshold" in alert.message


def test_warning_approaching_high():
    config = AnalyticsConfig(alert_temp_high=35.0, warning_margin=5.0)
    engine = AnalyticsEngine(config)
    alert, _ = engine.process(31.0, "sensor-1")
    assert alert is not None
    assert alert.level == AlertLevel.WARNING


def test_rolling_stats():
    engine = AnalyticsEngine(AnalyticsConfig(rolling_window_size=5))
    temps = [20.0, 21.0, 22.0, 23.0, 24.0]
    for t in temps:
        _, stats = engine.process(t, "sensor-1")

    assert stats.count == 5
    assert abs(stats.mean - 22.0) < 0.01
    assert stats.min_val == 20.0
    assert stats.max_val == 24.0


def test_counter_tracking():
    engine = AnalyticsEngine(AnalyticsConfig(alert_temp_high=30.0))
    engine.process(22.0, "s1")
    engine.process(31.0, "s1")  # alert
    engine.process(22.0, "s1")

    assert engine.total_processed == 3
    assert engine.total_alerts == 1


if __name__ == "__main__":
    test_normal_reading_no_alert()
    test_high_threshold_critical()
    test_low_threshold_critical()
    test_warning_approaching_high()
    test_rolling_stats()
    test_counter_tracking()
    print("All tests passed!")
