#include "WeatherManager.h"
//#include "MultiDisplay.h"
#include "MultiDisplay.h"
#include "Button.h"
#include "ino_compat.h"

#include <pico/stdlib.h>
#include <pico/binary_info.h>
#include <pico/multicore.h>
#include <hardware/adc.h>

#include <iostream>
#include <vector>
#include <array>

void displayThread()
{
    weather_station::MultiDisplay md(
        11, 12, {16, 13, 19, 10}, {8 + 2, 8 + 5, 8 + 6, 2}, {8 + 3, 8 + 7, 4, 6, 7, 8 + 4, 3, 5}
    );
    md.setNumber(0, 1000);
    md.setNumber(1, 2000);
    md.setNumber(2, 3000);
    md.setNumber(3, 4000);
    int received = 0;
    std::array<uint32_t, 4> values;
    auto last = millis();
    for (;;) {
        md.refreshDisplay();
        auto now = millis();
        if (now - last > 100) {
            bool res = multicore_fifo_rvalid();
            if (res) {
                values[received] = multicore_fifo_pop_blocking();
                ++received;
                if (received == 4) {
                    md.setNumber(0, values[0]);
                    union {
                        float f;
                        uint32_t ui;
                    } temp, hum, obt;
                    temp.ui = values[1];
                    md.setNumberF(1, temp.f, 2);
                    md.setSegment(1, 3, 0b01011000);
                    hum.ui = values[2];
                    md.setNumberF(2, hum.f, 2);
                    obt.ui = values[3];
                    md.setNumberF(3, obt.f, 2);
                    md.setSegment(3, 3, 0b01011000);

                    received = 0;
                }
            }
            last = now;
        }
    }
}

float read_onboard_temperature()
{
    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    return tempC;
}

void processingThread()
{
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    constexpr int dhtPin = 15;
    weather_station::WeatherManager weather(dhtPin);

    uint64_t lastSync = 0;
    printf("Waiting for first measurement... (5 sec)\n");
    sleep_ms(500);
    uint64_t lastFps = millis();
    uint64_t lastTemp = 0;

    std::array<weather_station::Button, 3> buttons = {
        weather_station::Button{
            7,
            [&weather] {
                static int i = 0;
                weather.switchDisplay();
                std::cout << "Button 1\n";
            }
        },
        weather_station::Button{8, [] { std::cout << "Button 2\n"; }},
        weather_station::Button{9, [] { std::cout << "Button 3\n"; }}
    };
    uint64_t lastReady = 0;
    float onboardTemp = 0;

    for (;;) {
        auto now = millis();
        for (auto& btn : buttons) {
            btn.Process();
        }
        if (now - lastSync > 500) {
            auto co2 = weather.CO2();
            union {
                float f;
                uint32_t ui;
            } temp, hum, onboardT;
            temp.f = weather.temperature();
            hum.f = weather.humidity();
            onboardT.f = onboardTemp;
            multicore_fifo_push_blocking(co2);
            multicore_fifo_push_blocking(temp.ui);
            multicore_fifo_push_blocking(hum.ui);
            multicore_fifo_push_blocking(onboardT.ui);
            lastSync = millis();
        }
        if (now - lastTemp > 20000) {
            onboardTemp = read_onboard_temperature();
            std::cout << "Onboard temp: " << onboardTemp << "\n";
            lastTemp = now;
        }

        lastReady = weather.process();
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(1000);
    std::cout << "Start!\n";
    multicore_launch_core1(displayThread);
    processingThread();
}
