#include "filter.h"
#include "json_parser.h"

#include <iostream>
#include <string>
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
    std::cerr << "\n[data_filter] Shutting down...\n";
    g_running = false;
}

static double get_env_double(const char* name, double default_val) {
    const char* val = std::getenv(name);
    if (val) {
        try { return std::stod(val); }
        catch (...) {}
    }
    return default_val;
}

#ifdef STANDALONE_MODE

// ─── Standalone mode: reads JSON from stdin, writes filtered JSON to stdout ───
int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    iot_edge::DataFilter::Config config;
    config.temp_min_valid = get_env_double("TEMP_MIN_VALID", -40.0);
    config.temp_max_valid = get_env_double("TEMP_MAX_VALID", 85.0);
    config.noise_threshold = get_env_double("NOISE_THRESHOLD", 0.5);

    iot_edge::DataFilter filter(config);

    std::cerr << "[data_filter] Starting in STANDALONE mode\n";
    std::cerr << "[data_filter] Valid range: [" << config.temp_min_valid
              << ", " << config.temp_max_valid << "] C\n";
    std::cerr << "---\n";

    std::string line;
    while (g_running && std::getline(std::cin, line)) {
        if (line.empty()) continue;

        auto msg = iot_edge::JsonParser::parse_sensor_message(line);
        if (!msg) {
            std::cerr << "[data_filter] WARNING: Failed to parse message\n";
            continue;
        }

        auto result = filter.evaluate(msg->temperature);

        if (result.accepted) {
            // Forward clean data to stdout
            std::cout << iot_edge::JsonParser::to_json(*msg, true) << std::endl;
        } else {
            std::cerr << "[data_filter] Rejected seq=" << msg->sequence_number
                      << " temp=" << msg->temperature
                      << " reason=" << result.reason << "\n";
        }
    }

    std::cerr << "[data_filter] Stats: total=" << filter.total_count()
              << " accepted=" << filter.accepted_count()
              << " rejected=" << filter.rejected_count() << "\n";
    std::cerr << "[data_filter] Stopped.\n";
    return 0;
}

#else

// ─── IoT Edge mode: receives from Edge Hub input, sends to output ───

static iot_edge::DataFilter* g_filter = nullptr;
static IOTHUB_MODULE_CLIENT_LL_HANDLE g_client = nullptr;

static IOTHUBMESSAGE_DISPOSITION_RESULT input_message_callback(
    IOTHUB_MESSAGE_HANDLE message, void* /*userContext*/)
{
    const unsigned char* buffer;
    size_t size;

    if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK) {
        std::cerr << "[data_filter] WARNING: Could not get message bytes\n";
        return IOTHUBMESSAGE_REJECTED;
    }

    std::string json(reinterpret_cast<const char*>(buffer), size);
    auto msg = iot_edge::JsonParser::parse_sensor_message(json);

    if (!msg) {
        std::cerr << "[data_filter] WARNING: Failed to parse message\n";
        return IOTHUBMESSAGE_REJECTED;
    }

    auto result = g_filter->evaluate(msg->temperature);

    if (result.accepted) {
        std::string output_json = iot_edge::JsonParser::to_json(*msg, true);
        IOTHUB_MESSAGE_HANDLE output_msg =
            IoTHubMessage_CreateFromString(output_json.c_str());

        if (output_msg) {
            IoTHubMessage_SetContentTypeSystemProperty(output_msg, "application/json");
            IoTHubMessage_SetContentEncodingSystemProperty(output_msg, "utf-8");
            IoTHubMessage_SetProperty(output_msg, "source", "dataFilter");
            IoTHubMessage_SetProperty(output_msg, "filterPassed", "true");

            IoTHubModuleClient_LL_SendEventToOutputAsync(
                g_client, output_msg, "filterOutput", nullptr, nullptr);

            IoTHubMessage_Destroy(output_msg);
        }
    }

    return IOTHUBMESSAGE_ACCEPTED;
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::cerr << "[data_filter] Starting in IoT Edge mode\n";

    if (platform_init() != 0) {
        std::cerr << "[data_filter] ERROR: platform_init failed\n";
        return 1;
    }

    g_client = IoTHubModuleClient_LL_CreateFromEnvironment(MQTT_Protocol);
    if (!g_client) {
        std::cerr << "[data_filter] ERROR: Could not create module client\n";
        platform_deinit();
        return 1;
    }

    iot_edge::DataFilter::Config config;
    config.temp_min_valid = get_env_double("TEMP_MIN_VALID", -40.0);
    config.temp_max_valid = get_env_double("TEMP_MAX_VALID", 85.0);
    config.noise_threshold = get_env_double("NOISE_THRESHOLD", 0.5);

    iot_edge::DataFilter filter(config);
    g_filter = &filter;

    IoTHubModuleClient_LL_SetInputMessageCallback(
        g_client, "filterInput", input_message_callback, nullptr);

    while (g_running) {
        IoTHubModuleClient_LL_DoWork(g_client);
        ThreadAPI_Sleep(100);
    }

    IoTHubModuleClient_LL_Destroy(g_client);
    platform_deinit();

    std::cerr << "[data_filter] Stats: total=" << filter.total_count()
              << " accepted=" << filter.accepted_count()
              << " rejected=" << filter.rejected_count() << "\n";
    std::cerr << "[data_filter] Stopped.\n";
    return 0;
}

#endif
