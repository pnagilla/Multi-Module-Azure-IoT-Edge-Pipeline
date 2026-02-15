#include "sensor.h"
#include "message_builder.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <atomic>

#ifndef STANDALONE_MODE
#include "iothub_module_client_ll.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/platform.h"
#include "iothubtransportmqtt.h"
#endif

static std::atomic<bool> g_running{true};

static void signal_handler(int sig) {
    (void)sig;
    std::cerr << "\n[sensor_simulator] Shutting down...\n";
    g_running = false;
}

static int get_env_int(const char* name, int default_val) {
    const char* val = std::getenv(name);
    if (val) {
        try { return std::stoi(val); }
        catch (...) {}
    }
    return default_val;
}

static std::string get_env_str(const char* name, const std::string& default_val) {
    const char* val = std::getenv(name);
    return val ? std::string(val) : default_val;
}

#ifdef STANDALONE_MODE

// ─── Standalone mode: prints to stdout, useful for local development ───
int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    int interval_ms = get_env_int("TELEMETRY_INTERVAL_MS", 3000);
    std::string sensor_id = get_env_str("SENSOR_ID", "temp-sensor-001");

    std::cerr << "[sensor_simulator] Starting in STANDALONE mode\n";
    std::cerr << "[sensor_simulator] Sensor ID: " << sensor_id << "\n";
    std::cerr << "[sensor_simulator] Interval: " << interval_ms << " ms\n";
    std::cerr << "---\n";

    iot_edge::TemperatureSensor sensor(sensor_id);

    while (g_running) {
        auto reading = sensor.read();
        auto msg = iot_edge::MessageBuilder::build(reading);

        // In standalone mode, write JSON to stdout (can be piped to data_filter)
        std::cout << msg.body << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }

    std::cerr << "[sensor_simulator] Stopped.\n";
    return 0;
}

#else

// ─── IoT Edge mode: sends messages via Edge Hub ───
int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    int interval_ms = get_env_int("TELEMETRY_INTERVAL_MS", 3000);
    std::string sensor_id = get_env_str("SENSOR_ID", "temp-sensor-001");

    std::cerr << "[sensor_simulator] Starting in IoT Edge mode\n";

    if (platform_init() != 0) {
        std::cerr << "[sensor_simulator] ERROR: platform_init failed\n";
        return 1;
    }

    IOTHUB_MODULE_CLIENT_LL_HANDLE client =
        IoTHubModuleClient_LL_CreateFromEnvironment(MQTT_Protocol);

    if (client == nullptr) {
        std::cerr << "[sensor_simulator] ERROR: Could not create module client\n";
        platform_deinit();
        return 1;
    }

    iot_edge::TemperatureSensor sensor(sensor_id);

    while (g_running) {
        auto reading = sensor.read();
        auto msg = iot_edge::MessageBuilder::build(reading);

        IOTHUB_MESSAGE_HANDLE message_handle =
            IoTHubMessage_CreateFromString(msg.body.c_str());

        if (message_handle != nullptr) {
            IoTHubMessage_SetContentTypeSystemProperty(message_handle, "application/json");
            IoTHubMessage_SetContentEncodingSystemProperty(message_handle, "utf-8");
            IoTHubMessage_SetProperty(message_handle, "source", msg.source.c_str());

            IOTHUB_CLIENT_RESULT result =
                IoTHubModuleClient_LL_SendEventToOutputAsync(
                    client, message_handle, "sensorOutput", nullptr, nullptr);

            if (result != IOTHUB_CLIENT_OK) {
                std::cerr << "[sensor_simulator] WARNING: Send failed, result=" << result << "\n";
            }

            IoTHubMessage_Destroy(message_handle);
        }

        IoTHubModuleClient_LL_DoWork(client);
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
    }

    IoTHubModuleClient_LL_Destroy(client);
    platform_deinit();

    std::cerr << "[sensor_simulator] Stopped.\n";
    return 0;
}

#endif
