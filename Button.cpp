#include "Button.h"

#include <pico/stdlib.h>
#include <pico/binary_info.h>

#include <iostream>

namespace weather_station
{
Button::Button(int pin, std::function<void(void)> f)
    : pin_(pin)
    , f_(f)
{
    gpio_init(pin_);
    gpio_set_dir(pin_, GPIO_IN);
    gpio_pull_up(pin_);
}
void Button::Process()
{
    auto status = gpio_get(pin_);
    if (status != status_) {
        status_ = status;
        if (status == 1) {
            f_();
        }
    }
}
} // namespace weather_station