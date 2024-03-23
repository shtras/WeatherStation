#pragma once

#include "dht_nonblocking.h"
#include "SCD.h"

#include "MultiDisplay.h"

#include <vector>
#include <array>
#include <memory>

namespace weather_station
{
class WeatherManager
{
public:
explicit WeatherManager(int dhtPin);
    bool process();

    void display(MultiDisplay& md);

private:
    std::vector<std::unique_ptr<Sensor>> sensors_;
    std::vector<Sensor::Measurement> measurements_;    

    int displayedSensor_ = 0;
    uint64_t lastSwitch_ = 0;
};
} // namespace weather_station
