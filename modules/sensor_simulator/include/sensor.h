#pragma once

#include <string>
#include <random>
#include <chrono>

namespace iot_edge {

/// Simulates a temperature sensor with realistic noise and drift patterns.
class TemperatureSensor {
public:
    struct Reading {
        double temperature_celsius;
        double humidity_percent;
        std::string sensor_id;
        std::string timestamp;  // ISO 8601
        uint64_t sequence_number;
    };

    explicit TemperatureSensor(const std::string& sensor_id,
                                double base_temp = 22.0,
                                double noise_amplitude = 2.0);

    /// Generate the next sensor reading with simulated drift and noise.
    Reading read();

    /// Reset the sensor simulation state.
    void reset();

private:
    std::string sensor_id_;
    double base_temp_;
    double noise_amplitude_;
    uint64_t sequence_ = 0;

    std::mt19937 rng_;
    std::normal_distribution<double> noise_dist_;
    std::uniform_real_distribution<double> humidity_dist_;

    // Simulated drift state
    double drift_ = 0.0;
    double drift_velocity_ = 0.0;

    static std::string current_timestamp();
    void update_drift();
};

}  // namespace iot_edge
