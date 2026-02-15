#pragma once

#include <string>
#include <deque>

namespace iot_edge {

/// Filter result with reason for rejection.
struct FilterResult {
    bool accepted;
    std::string reason;  // empty if accepted
};

/// Validates and filters sensor data, rejecting out-of-range or noisy readings.
class DataFilter {
public:
    struct Config {
        double temp_min_valid = -40.0;    // sensor physical minimum
        double temp_max_valid = 85.0;     // sensor physical maximum
        double noise_threshold = 0.5;     // max allowed rate of change per reading
        size_t spike_window = 5;          // number of readings for spike detection
    };

    DataFilter();
    explicit DataFilter(const Config& config);

    /// Evaluate a temperature reading. Returns whether it should pass through.
    FilterResult evaluate(double temperature);

    /// Get count of total/accepted/rejected readings.
    uint64_t total_count() const { return total_; }
    uint64_t accepted_count() const { return accepted_; }
    uint64_t rejected_count() const { return rejected_; }

private:
    Config config_;
    std::deque<double> recent_readings_;
    uint64_t total_ = 0;
    uint64_t accepted_ = 0;
    uint64_t rejected_ = 0;

    bool is_in_range(double temp) const;
    bool is_spike(double temp) const;
};

}  // namespace iot_edge
