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
    uint64_t process();

    void switchDisplay();
    int CO2();
    float temperature();
    float humidity();

private:
    std::vector<std::unique_ptr<Sensor>> sensors_;
    std::vector<Sensor::Measurement> measurements_;
    std::vector<uint64_t> lastMeasurement_;

    int displayedSensor_ = 1;
};
} // namespace weather_station
