cmake_minimum_required(VERSION 3.24)
include(pico_sdk_import.cmake)
project(WeatherStation C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

add_executable(weather_station
        main.cpp
        MultiDisplay.cpp
        MultiDisplay.cpp
        embedded-i2c-scd4x/scd4x_i2c.cpp
        embedded-i2c-scd4x/sensirion_common.c
        embedded-i2c-scd4x/sensirion_i2c_hal.c
        embedded-i2c-scd4x/sensirion_i2c.c
        dht_nonblocking.cpp
        WeatherManager.cpp
        SCD.cpp
        Button.cpp
        Comm.cpp
        TCP.cpp
        MQTT.cpp
        )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")

target_include_directories(weather_station PRIVATE
        ${CMAKE_CURRENT_LIST_DIR})

# pull in common dependencies
target_link_libraries(weather_station pico_stdlib pico_multicore hardware_i2c hardware_adc pico_cyw43_arch_lwip_threadsafe_background pico_lwip_mqtt)

target_compile_definitions(weather_station PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
    MQTT_SERVER=\"${MQTT_SERVER}\"
    MQTT_USERNAME=\"${MQTT_USERNAME}\"
    MQTT_PASSWORD=\"${MQTT_PASSWORD}\"
    )

# create map/bin/hex file etc.
pico_add_extra_outputs(weather_station)

# enable usb output, disable uart output
pico_enable_stdio_usb(weather_station 1)
pico_enable_stdio_uart(weather_station 0)