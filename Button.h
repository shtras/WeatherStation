#pragma once

#include <functional>

namespace weather_station
{
class Button
{
public:
    explicit Button(int pin, std::function<void(void)> f);

    void Process();

private:
    int pin_ = 0;
    std::function<void(void)> f_;
    int status_ = 1;
};
} // namespace weather_station
