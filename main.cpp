#include "WeatherManager.h"
//#include "MultiDisplay.h"
#include "MultiDisplay.h"
#include "Button.h"
#include "Comm.h"
//#include "TCP.h"
#include "MQTT.h"
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
    weather_station::Receiver receiver;
    auto last = millis();
    for (;;) {
        md.refreshDisplay();
        auto now = millis();
        auto msg = receiver.process();
        if (!msg) {
            continue;
        }
        if (msg->type == weather_station::Message::Type::WeatherInfo) {
            md.setNumber(0, msg->data[0]);
            md.setNumberF(1, reinterpret_cast<float&>(msg->data[1]), 2);
            md.setSegment(1, 3, 0b01011000);
            md.setNumberF(2, reinterpret_cast<float&>(msg->data[2]), 2);
            md.setNumberF(3, reinterpret_cast<float&>(msg->data[3]), 2);
            md.setSegment(3, 3, 0b01011000);
        } else if (msg->type == weather_station::Message::Type::IncDelay) {
            md.incDelay();
        } else if (msg->type == weather_station::Message::Type::DecDelay) {
            md.decDelay();
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
    //weather_station::TCPTest tcp;
    weather_station::MQTT mqtt;

    mqtt.Connect();

    //for (;;);

    std::array<weather_station::Button, 3> buttons = {
        weather_station::Button{
            17,
            [&weather] {
                static int i = 0;
                //weather.switchDisplay();
                std::cout << "Button 1\n";
            }
        },
        weather_station::Button{
            18,
            [] {
                std::cout << "Button 2\n";
                multicore_fifo_push_blocking(static_cast<uint32_t>(weather_station::Message::Type::IncDelay));
            }
        },
        weather_station::Button{20, [] {
                                    std::cout << "Button 3\n";
                                    multicore_fifo_push_blocking(
                                        static_cast<uint32_t>(weather_station::Message::Type::DecDelay)
                                    );
                                }}
    };
    uint64_t lastReady = 0;
    float onboardTemp = 0;
    uint64_t lastTcpReport = millis();

    for (;;) {
        auto now = millis();
        for (auto& btn : buttons) {
            btn.Process();
        }
        if (now - lastSync > 2000) {
            auto co2 = weather.CO2();
            union {
                float f;
                uint32_t ui;
            } temp, hum, onboardT;
            temp.f = weather.temperature();
            hum.f = weather.humidity();
            onboardT.f = onboardTemp;
            multicore_fifo_push_blocking(static_cast<uint32_t>(weather_station::Message::Type::WeatherInfo));
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

        if (now - lastTcpReport > 60000) {
            lastTcpReport = now;
            mqtt.ReportWeather(weather.CO2(), weather.temperature(), weather.humidity());
        }
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);
    std::cout << "Start!\n";
    multicore_launch_core1(displayThread);
    processingThread();
}
