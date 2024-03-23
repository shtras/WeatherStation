/*
 * DHT11, DHT21, and DHT22 non-blocking library.
 * Based on Adafruit Industries' DHT driver library.
 *
 * (C) 2015 Ole Wolf <wolf@blazingangles.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "Sensor.h"

#include <stdint.h>
#include <hardware/sync.h>
namespace weather_station
{
class DHT_nonblocking: public Sensor
{
public:
    enum class Type { DHT_TYPE_11 = 0, DHT_TYPE_21 = 1, DHT_TYPE_22 = 2 };
    DHT_nonblocking(uint8_t pin, Type type);
    bool process();

private:
    bool measure(float* temperature, float* humidity);

    bool read_data();
    bool read_nonblocking();
    float read_temperature() const;
    float read_humidity() const;
    uint32_t expect_pulse(bool level) const;

    uint8_t dht_state;
    unsigned long dht_timestamp;
    uint8_t data[6];
    const uint8_t _pin;
    Type _type;
    const uint64_t _maxcycles;
    uint64_t lastMEasurement_ = 0;
};

class DHT_interrupt
{
public:
    DHT_interrupt()
    {
        interruptsState_ = save_and_disable_interrupts();
    }
    ~DHT_interrupt()
    {
        restore_interrupts(interruptsState_);
    }

private:
    uint32_t interruptsState_ = 0;
};
} // namespace weather_station
