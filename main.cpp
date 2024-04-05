#include "WeatherManager.h"
#include "MultiDisplay.h"
#include "Button.h"
#include "ino_compat.h"

#include <pico/stdlib.h>
#include <pico/binary_info.h>

#include <iostream>
#include <vector>
#include <array>

void foo()
{
    MultiDisplay md(
        11, 12, {16, 13, 19, 10}, {8 + 2, 8 + 5, 8 + 6, 2}, {8 + 3, 8 + 7, 4, 6, 7, 8 + 4, 3, 5}
    );
    md.begin();

    constexpr int dhtPin = 15;
    weather_station::WeatherManager weather(dhtPin);

    uint64_t last = 0;
    printf("Waiting for first measurement... (5 sec)\n");
    sleep_ms(500);
    int iters = 0;
    uint64_t lastFps = millis();

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

    for (;;) {
        ++iters;
        auto now = millis();
        md.refreshDisplay();
        for (auto& btn : buttons) {
            btn.Process();
        }
        if (iters >= 1000000ULL) {
            auto fps = (float)iters / 10.0f / (float)(now - lastFps);
            std::cout << "FPS: " << fps << "\n";
            iters = 0;
            lastFps = now;
        }
        last = now;

        auto lastReady = weather.process();
        //md.setNumber(3, (now - lastReady) / 100);

        weather.display(md);
    }
}

int main()
{
    stdio_init_all();
    //sleep_ms(10000);
    std::cout << "Start!\n";
    foo();
}
