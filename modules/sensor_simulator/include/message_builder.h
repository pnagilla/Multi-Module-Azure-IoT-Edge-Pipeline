#pragma once

#include "sensor.h"
#include <string>

namespace iot_edge {

/// Builds JSON messages from sensor readings for IoT Edge Hub.
class MessageBuilder {
public:
    /// Serialize a sensor reading to JSON string.
    static std::string to_json(const TemperatureSensor::Reading& reading);

    /// Create a message with metadata properties for IoT Edge routing.
    struct Message {
        std::string body;           // JSON payload
        std::string content_type;   // application/json
        std::string source;         // module name for routing
    };

    static Message build(const TemperatureSensor::Reading& reading);
};

}  // namespace iot_edge
