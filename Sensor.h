#pragma once

#include <stdint.h>

namespace weather_station
{
class Sensor
{
public:
    virtual bool process() = 0;
    struct Measurement
    {
        float Temperature = 0;
        float Humidity = 0;
        uint16_t CO2 = 0;
    };

    Measurement& GetMeasurement()
    {
        return measurement_;
    }

protected:
    Measurement measurement_;
};
} // namespace weather_station
