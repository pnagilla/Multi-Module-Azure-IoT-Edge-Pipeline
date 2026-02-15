#include "message_builder.h"
#include <sstream>
#include <iomanip>

namespace iot_edge {

std::string MessageBuilder::to_json(const TemperatureSensor::Reading& reading) {
    // Manual JSON construction to avoid external dependency.
    // For production with complex schemas, consider nlohmann/json or rapidjson.
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "{"
        << "\"sensorId\":\"" << reading.sensor_id << "\","
        << "\"temperature\":" << reading.temperature_celsius << ","
        << "\"humidity\":" << std::setprecision(1) << reading.humidity_percent << ","
        << "\"timestamp\":\"" << reading.timestamp << "\","
        << "\"sequenceNumber\":" << reading.sequence_number
        << "}";
    return oss.str();
}

MessageBuilder::Message MessageBuilder::build(const TemperatureSensor::Reading& reading) {
    Message msg;
    msg.body = to_json(reading);
    msg.content_type = "application/json";
    msg.source = "sensorSimulator";
    return msg;
}

}  // namespace iot_edge
