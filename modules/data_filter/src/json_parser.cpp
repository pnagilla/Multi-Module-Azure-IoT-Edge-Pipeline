#include "json_parser.h"
#include <sstream>
#include <iomanip>

namespace iot_edge {

std::optional<SensorMessage> JsonParser::parse_sensor_message(const std::string& json) {
    auto sensor_id = extract_string(json, "sensorId");
    auto temperature = extract_number(json, "temperature");
    auto humidity = extract_number(json, "humidity");
    auto timestamp = extract_string(json, "timestamp");
    auto sequence = extract_uint(json, "sequenceNumber");

    if (!sensor_id || !temperature || !humidity || !timestamp || !sequence) {
        return std::nullopt;
    }

    SensorMessage msg;
    msg.sensor_id = *sensor_id;
    msg.temperature = *temperature;
    msg.humidity = *humidity;
    msg.timestamp = *timestamp;
    msg.sequence_number = *sequence;

    return msg;
}

std::string JsonParser::to_json(const SensorMessage& msg, bool filter_passed,
                                 const std::string& filter_reason) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "{"
        << "\"sensorId\":\"" << msg.sensor_id << "\","
        << "\"temperature\":" << msg.temperature << ","
        << "\"humidity\":" << std::setprecision(1) << msg.humidity << ","
        << "\"timestamp\":\"" << msg.timestamp << "\","
        << "\"sequenceNumber\":" << msg.sequence_number << ","
        << "\"filterPassed\":" << (filter_passed ? "true" : "false");

    if (!filter_reason.empty()) {
        oss << ",\"filterReason\":\"" << filter_reason << "\"";
    }

    oss << "}";
    return oss.str();
}

std::optional<std::string> JsonParser::extract_string(const std::string& json,
                                                       const std::string& key) {
    std::string search = "\"" + key + "\":\"";
    auto pos = json.find(search);
    if (pos == std::string::npos) return std::nullopt;

    pos += search.length();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return std::nullopt;

    return json.substr(pos, end - pos);
}

std::optional<double> JsonParser::extract_number(const std::string& json,
                                                  const std::string& key) {
    std::string search = "\"" + key + "\":";
    auto pos = json.find(search);
    if (pos == std::string::npos) return std::nullopt;

    pos += search.length();
    try {
        size_t processed = 0;
        double val = std::stod(json.substr(pos), &processed);
        if (processed == 0) return std::nullopt;
        return val;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<uint64_t> JsonParser::extract_uint(const std::string& json,
                                                  const std::string& key) {
    std::string search = "\"" + key + "\":";
    auto pos = json.find(search);
    if (pos == std::string::npos) return std::nullopt;

    pos += search.length();
    try {
        size_t processed = 0;
        uint64_t val = std::stoull(json.substr(pos), &processed);
        if (processed == 0) return std::nullopt;
        return val;
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace iot_edge
