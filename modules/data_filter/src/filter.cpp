#include "filter.h"
#include <cmath>
#include <numeric>

namespace iot_edge {

DataFilter::DataFilter()
    : config_()
{
}

DataFilter::DataFilter(const Config& config)
    : config_(config)
{
}

FilterResult DataFilter::evaluate(double temperature) {
    total_++;

    // Check 1: Physical range validation
    if (!is_in_range(temperature)) {
        rejected_++;
        return {false, "out_of_range"};
    }

    // Check 2: Spike detection (sudden jumps likely indicate sensor error)
    if (recent_readings_.size() >= 2 && is_spike(temperature)) {
        rejected_++;
        // Still add to window so recovery readings aren't also flagged
        recent_readings_.push_back(temperature);
        if (recent_readings_.size() > config_.spike_window) {
            recent_readings_.pop_front();
        }
        return {false, "spike_detected"};
    }

    // Reading passed all checks
    recent_readings_.push_back(temperature);
    if (recent_readings_.size() > config_.spike_window) {
        recent_readings_.pop_front();
    }

    accepted_++;
    return {true, ""};
}

bool DataFilter::is_in_range(double temp) const {
    return temp >= config_.temp_min_valid && temp <= config_.temp_max_valid;
}

bool DataFilter::is_spike(double temp) const {
    if (recent_readings_.empty()) return false;

    // Calculate running average
    double avg = std::accumulate(recent_readings_.begin(), recent_readings_.end(), 0.0)
                 / static_cast<double>(recent_readings_.size());

    // A spike is a reading that deviates more than noise_threshold * stdev from the mean
    double variance = 0.0;
    for (double r : recent_readings_) {
        double diff = r - avg;
        variance += diff * diff;
    }
    variance /= static_cast<double>(recent_readings_.size());
    double stdev = std::sqrt(variance);

    // Minimum stdev to avoid false positives on stable readings
    double effective_stdev = std::max(stdev, 1.0);
    double deviation = std::abs(temp - avg);

    return deviation > (config_.noise_threshold * effective_stdev * 5.0);
}

}  // namespace iot_edge
