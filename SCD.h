#pragma once
#include "Sensor.h"

namespace weather_station
{
class SCD: public Sensor
{
public:
    SCD();
    bool process();

private:
    uint64_t lastMeasure_ = 0;
};
} // namespace weather_station
