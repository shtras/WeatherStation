#pragma once

#include "pico/stdlib.h"

#define HIGH 1
#define LOW 0
#define OUTPUT GPIO_OUT

extern "C" {
inline uint64_t micros()
{
    return time_us_64();
}

inline uint64_t millis()
{
    return micros() / 1000ULL;
}

inline void digitalWrite(int pin, int value)
{
    gpio_put(pin, value);
}

inline long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline long constrain(long val, long low, long high)
{
    if (val < low) {
        val = low;
    }
    if (val > high) {
        val = high;
    }
    return val;
}

inline void pinMode(int pin, int mode)
{
    gpio_init(pin);
    gpio_set_dir(pin, mode);
}
}