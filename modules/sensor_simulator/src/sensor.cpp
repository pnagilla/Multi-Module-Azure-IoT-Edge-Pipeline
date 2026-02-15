#include "sensor.h"
#include <cmath>
#include <iomanip>
#include <sstream>

namespace iot_edge {

TemperatureSensor::TemperatureSensor(const std::string& sensor_id,
                                     double base_temp,
                                     double noise_amplitude)
    : sensor_id_(sensor_id)
    , base_temp_(base_temp)
    , noise_amplitude_(noise_amplitude)
    , rng_(std::random_device{}())
    , noise_dist_(0.0, noise_amplitude)
    , humidity_dist_(30.0, 70.0)
{
}

TemperatureSensor::Reading TemperatureSensor::read() {
    update_drift();

    double temp = base_temp_ + drift_ + noise_dist_(rng_);
    double humidity = humidity_dist_(rng_);

    // Clamp humidity to valid range
    humidity = std::max(0.0, std::min(100.0, humidity));

    Reading r;
    r.temperature_celsius = std::round(temp * 100.0) / 100.0;  // 2 decimal places
    r.humidity_percent = std::round(humidity * 10.0) / 10.0;    // 1 decimal place
    r.sensor_id = sensor_id_;
    r.timestamp = current_timestamp();
    r.sequence_number = sequence_++;

    return r;
}

void TemperatureSensor::reset() {
    sequence_ = 0;
    drift_ = 0.0;
    drift_velocity_ = 0.0;
}

void TemperatureSensor::update_drift() {
    // Brownian-motion-style drift to simulate realistic temperature changes.
    // Mean-reverting: pulls drift back toward zero over time.
    std::normal_distribution<double> drift_noise(0.0, 0.1);
    drift_velocity_ += drift_noise(rng_) - 0.05 * drift_;
    drift_velocity_ = std::max(-1.0, std::min(1.0, drift_velocity_));
    drift_ += drift_velocity_ * 0.1;
    drift_ = std::max(-10.0, std::min(10.0, drift_));
}

std::string TemperatureSensor::current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm_buf{};
    gmtime_r(&time_t_now, &tm_buf);

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count()
        << 'Z';
    return oss.str();
}

}  // namespace iot_edge
