#pragma once

#include <string>
#include <optional>

namespace iot_edge {

/// Minimal JSON parser for sensor messages.
/// Extracts known fields without a full JSON library dependency.
struct SensorMessage {
    std::string sensor_id;
    double temperature;
    double humidity;
    std::string timestamp;
    uint64_t sequence_number;
};

class JsonParser {
public:
    /// Parse a sensor JSON message. Returns nullopt on parse failure.
    static std::optional<SensorMessage> parse_sensor_message(const std::string& json);

    /// Re-serialize a sensor message to JSON with filter metadata.
    static std::string to_json(const SensorMessage& msg, bool filter_passed,
                                const std::string& filter_reason = "");

private:
    static std::optional<std::string> extract_string(const std::string& json,
                                                      const std::string& key);
    static std::optional<double> extract_number(const std::string& json,
                                                 const std::string& key);
    static std::optional<uint64_t> extract_uint(const std::string& json,
                                                 const std::string& key);
};

}  // namespace iot_edge
