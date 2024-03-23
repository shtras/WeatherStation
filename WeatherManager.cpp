#include "WeatherManager.h"
#include "ino_compat.h"

#include <iostream>

namespace weather_station
{
WeatherManager::WeatherManager(int dhtPin)
{
    //sensors_.emplace_back(
    //    std::make_unique<DHT_nonblocking>(dhtPin, DHT_nonblocking::Type::DHT_TYPE_11));
    sensors_.emplace_back(std::make_unique<SCD>());

    measurements_.resize(sensors_.size());
}

bool WeatherManager::process()
{
    bool res = false;
    for (int i = 0; i < sensors_.size(); ++i) {
        auto& sensor = sensors_[i];
        if (sensor->process()) {
            measurements_[i] = sensor->GetMeasurement();
            std::cout << "Sensor: " << i << " CO2: " << measurements_[i].CO2
                      << " Temp: " << measurements_[i].Temperature
                      << " Humidity: " << measurements_[i].Humidity << "\n";
            res = true;
        }
    }
    return res;
}

void WeatherManager::display(MultiDisplay& md)
{
    auto now = millis();
    if (now - lastSwitch_ > 2000) {
        lastSwitch_ = now;
        displayedSensor_ = (displayedSensor_ + 1) % sensors_.size();
    }

    const Sensor::Measurement& measurement = measurements_[displayedSensor_];
    md.setNumber(0, measurement.CO2);
    md.setNumberF(1, measurement.Temperature, 2);
    uint8_t segs[4];
    md.getSegments(1, segs);
    segs[3] = 0b01011000;
    md.setSegments(1, segs);
    md.setNumberF(2, measurement.Humidity, 2);
}
} // namespace weather_station
