cmake_minimum_required(VERSION 3.24)
include(pico_sdk_import.cmake)
project(WeatherStation C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

add_executable(weather_station
        main.cpp
        MultiDisplay.cpp
        embedded-i2c-scd4x/scd4x_i2c.cpp
        embedded-i2c-scd4x/sensirion_common.c
        embedded-i2c-scd4x/sensirion_i2c_hal.c
        embedded-i2c-scd4x/sensirion_i2c.c
        dht_nonblocking.cpp
        WeatherManager.cpp
        SCD.cpp
        Button.cpp
        )

# pull in common dependencies
target_link_libraries(weather_station pico_stdlib hardware_i2c)

# create map/bin/hex file etc.
pico_add_extra_outputs(weather_station)

# enable usb output, disable uart output
pico_enable_stdio_usb(weather_station 1)
pico_enable_stdio_uart(weather_station 0)