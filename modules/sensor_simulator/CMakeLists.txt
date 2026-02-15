cmake_minimum_required(VERSION 3.14)
project(sensor_simulator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Threads REQUIRED)

# Azure IoT SDK C (installed via vcpkg or system package)
# For local dev without the SDK, we compile in STANDALONE mode
option(STANDALONE_MODE "Build without Azure IoT SDK for local testing" ON)

add_executable(sensor_simulator
    src/main.cpp
    src/sensor.cpp
    src/message_builder.cpp
)

target_include_directories(sensor_simulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sensor_simulator PRIVATE
    Threads::Threads
)

if(STANDALONE_MODE)
    target_compile_definitions(sensor_simulator PRIVATE STANDALONE_MODE)
    message(STATUS "Building in STANDALONE mode (no Azure IoT SDK dependency)")
else()
    # Link against Azure IoT SDK when available
    find_package(azure_iot_sdks QUIET)
    if(azure_iot_sdks_FOUND)
        target_link_libraries(sensor_simulator PRIVATE
            iothub_client
            iothub_client_mqtt_transport
        )
    else()
        message(WARNING "Azure IoT SDK not found. Building in standalone mode.")
        target_compile_definitions(sensor_simulator PRIVATE STANDALONE_MODE)
    endif()
endif()

install(TARGETS sensor_simulator DESTINATION bin)
