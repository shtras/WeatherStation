#include "SCD.h"

#include "ino_compat.h"
#include "embedded-i2c-scd4x/sensirion_i2c_hal.h"
#include "embedded-i2c-scd4x/scd4x_i2c.h"
#include "hardware/i2c.h"

#include <iostream>

namespace weather_station
{
int16_t checkError(uint16_t err, std::string_view where)
{
    if (err != 0) {
        std::cout << "Error at " << where << ": " << err << "\n";
    }
    return err;
}
SCD::SCD()
{
    sensirion_i2c_hal_init();

    // Clean up potential SCD40 states
    checkError(scd4x_wake_up(), "scd4x_wake_up");
    checkError(scd4x_stop_periodic_measurement(), "scd4x_stop_periodic_measurement");
    sleep_ms(600);
    checkError(scd4x_reinit(), "scd4x_reinit");

    uint16_t serial_0;
    uint16_t serial_1;
    uint16_t serial_2;
    if (checkError(scd4x_get_serial_number(&serial_0, &serial_1, &serial_2),
            "scd4x_get_serial_number") == 0) {
        printf("serial: 0x%04x%04x%04x\n", serial_0, serial_1, serial_2);
    }

    //uint16_t status;
    //checkError(scd4x_perform_self_test(&status), "self_test");
    //std::cout << "Self test status: " << status << "\n";

    checkError(scd4x_start_periodic_measurement(), "scd4x_start_periodic_measurement");
    lastMeasure_ = millis() + 5000;
}

bool SCD::process()
{
    auto now = millis();
    if (lastMeasure_ > now || now - lastMeasure_ < 1000) {
        return false;
    }
    lastMeasure_ = now;

    bool data_ready_flag = false;
    auto err = scd4x_get_data_ready_flag(&data_ready_flag);
    if (err == -123) {
        ++numRestarts_;
        std::cout << "Restart " << numRestarts_ << "\n";
        sleep_ms(100);
        checkError(scd4x_power_down(), "power_down");
        sleep_ms(1000);
        checkError(scd4x_wake_up(), "scd4x_wake_up");
        checkError(scd4x_stop_periodic_measurement(), "scd4x_stop_periodic_measurement");
        sleep_ms(600);
        checkError(scd4x_reinit(), "scd4x_reinit");
        checkError(scd4x_start_periodic_measurement(), "scd4x_start_periodic_measurement");
        lastMeasure_ = now + 5000;
        return false;
    } else if (err != 0) {
        std::cout << "Unknown error\n";
        return false;
    }
    if (!data_ready_flag) {
        return false;
    }

    int32_t temperature = 0;
    int32_t humidity = 0;
    if (checkError(scd4x_read_measurement(&measurement_.CO2, &temperature, &humidity),
            "scd4x_read_measurement")) {
        return false;
    }
    if (measurement_.CO2 == 0) {
        std::cout << "Invalid sample detected, skipping.\n";
        return false;
    }
    measurement_.Temperature = temperature / 1000.0f;
    measurement_.Humidity = humidity / 1000.0f;
    return true;
}
} // namespace weather_station