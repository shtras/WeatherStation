cmake_minimum_required(VERSION 3.24)
include(pico_sdk_import.cmake)
project(Test C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

pico_sdk_init()

add_executable(test
        main.cpp
        MultiDisplay.cpp
        embedded-i2c-scd4x/scd4x_i2c.cpp
        embedded-i2c-scd4x/sensirion_common.c
        embedded-i2c-scd4x/sensirion_i2c_hal.c
        embedded-i2c-scd4x/sensirion_i2c.c
        dht_nonblocking.cpp
        WeatherManager.cpp
        SCD.cpp
        )

# pull in common dependencies
target_link_libraries(test pico_stdlib hardware_i2c)

# create map/bin/hex file etc.
pico_add_extra_outputs(test)

# enable usb output, disable uart output
pico_enable_stdio_usb(test 1)
pico_enable_stdio_uart(test 0)