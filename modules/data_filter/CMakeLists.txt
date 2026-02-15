cmake_minimum_required(VERSION 3.14)
project(data_filter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

option(STANDALONE_MODE "Build without Azure IoT SDK for local testing" ON)

add_executable(data_filter
    src/main.cpp
    src/filter.cpp
    src/json_parser.cpp
)

target_include_directories(data_filter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(data_filter PRIVATE
    Threads::Threads
)

if(STANDALONE_MODE)
    target_compile_definitions(data_filter PRIVATE STANDALONE_MODE)
    message(STATUS "Building in STANDALONE mode (no Azure IoT SDK dependency)")
else()
    find_package(azure_iot_sdks QUIET)
    if(azure_iot_sdks_FOUND)
        target_link_libraries(data_filter PRIVATE
            iothub_client
            iothub_client_mqtt_transport
        )
    else()
        message(WARNING "Azure IoT SDK not found. Building in standalone mode.")
        target_compile_definitions(data_filter PRIVATE STANDALONE_MODE)
    endif()
endif()

install(TARGETS data_filter DESTINATION bin)
