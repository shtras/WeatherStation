#include "WeatherManager.h"
#include "ino_compat.h"

#include <iostream>
#include <algorithm>

namespace weather_station
{
WeatherManager::WeatherManager(int dhtPin)
{
    sensors_.emplace_back(
        std::make_unique<DHT_nonblocking>(dhtPin, DHT_nonblocking::Type::DHT_TYPE_11)
    );
    sensors_.emplace_back(std::make_unique<SCD>());

    measurements_.resize(sensors_.size());
    lastMeasurement_.resize(sensors_.size());
}

uint64_t WeatherManager::process()
{
    for (int i = 0; i < sensors_.size(); ++i) {
        auto& sensor = sensors_[i];
        if (sensor->process()) {
            measurements_[i] = sensor->GetMeasurement();
            std::cout << "Sensor: " << i << " CO2: " << measurements_[i].CO2
                      << " Temp: " << measurements_[i].Temperature
                      << " Humidity: " << measurements_[i].Humidity << "\n";
            if (measurements_[i].CO2 == 0) {
                measurements_[i].CO2 = (*std::max_element(
                                            measurements_.begin(), measurements_.end(),
                                            [](auto& a, auto& b) { return a.CO2 < b.CO2; }
                                        )).CO2;
            }
            lastMeasurement_[i] = millis();
        }
    }
    return *std::min_element(lastMeasurement_.begin(), lastMeasurement_.end());
}

/*
void WeatherManager::display(MultiDisplay& md)
{
    const Sensor::Measurement& measurement = measurements_[displayedSensor_];
    md.setNumber(0, measurement.CO2);
    md.setNumberF(1, measurement.Temperature, 2);
    md.setSegment(1, 3, 0b01011000);
    md.setNumberF(2, measurement.Humidity, 2);
}
*/

int WeatherManager::CO2()
{
    const auto& measurement = measurements_[displayedSensor_];
    return measurement.CO2;
}

float WeatherManager::temperature()
{
    const auto& measurement = measurements_[displayedSensor_];
    return measurement.Temperature;
}

float WeatherManager::humidity()
{
    const auto& measurement = measurements_[displayedSensor_];
    return measurement.Humidity;
}

void WeatherManager::switchDisplay()
{
    displayedSensor_ = (displayedSensor_ + 1) % sensors_.size();
}
} // namespace weather_station
