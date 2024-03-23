#include <iostream>
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "ino_compat.h"
#include "dht_nonblocking.h"

#include "WeatherManager.h"
#include "MultiDisplay.h"

void foo()
{
    MultiDisplay md(
        11, 12, {13, 16, 19, 10}, {8 + 2, 8 + 5, 8 + 6, 2}, {8 + 3, 8 + 7, 4, 6, 7, 8 + 4, 3, 5});
    md.begin();

    constexpr int dhtPin = 15;
    weather_station::WeatherManager weather(dhtPin);
    

    uint64_t last = 0;
    printf("Waiting for first measurement... (5 sec)\n");
    sleep_ms(500);
    int iters = 0;
    uint64_t lastFps = millis();
    uint64_t lastDht = millis();
    uint64_t lastReady = millis();
    for (;;) {
        ++iters;
        auto now = millis();
        md.refreshDisplay();
        if (iters >= 1000000ULL) {
            auto fps = (float)iters / 10.0f / (float)(now - lastFps);
            std::cout << "FPS: " << fps << " Since ready: " << now - lastReady << "\n";
            iters = 0;
            lastFps = now;
        }
        md.setNumber(3, (now-lastReady)/100);
        last = now;
        
        bool ready = weather.process();
        if (ready) {
            lastReady = now;
        }
        weather.display(md);
    }
}


int main()
{
    stdio_init_all();
    sleep_ms(10000);
    std::cout << "Start!\n";
    foo();
}
