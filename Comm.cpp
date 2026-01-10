#include "Comm.h"

#include <pico/stdlib.h>
#include <pico/multicore.h>

#include <iostream>
#include <cstring>

namespace weather_station
{
namespace
{
const std::map<Message::Type, int> messageSize = {
    {Message::Type::WeatherInfo, 4},
    {Message::Type::IncDelay, 0},
    {Message::Type::DecDelay, 0}
};
}

Message* Receiver::process()
{
    if (!multicore_fifo_rvalid()) {
        return nullptr;
    }
    switch (state_) {
        case State::Idle: {
            uint32_t val = multicore_fifo_pop_blocking();
            Message::Type type = static_cast<Message::Type>(val);
            //std::cout << "Now receiving " << val << "\n";
            message_.type = type;
            if (messageSize.count(type) == 0) {
                std::cout << "Bad message type: " << val << "\n";
                return nullptr;
            }
            toReceive_ = messageSize.at(type);
            if (toReceive_ == 0) {
                return &message_;
            }
            state_ = State::Receiving;
            return nullptr;
        }
        case State::Receiving: {
            message_.data[received_++] = multicore_fifo_pop_blocking();
            //std::cout << "Received " << received_ << " out of " << toReceive_ << "\n";
            if (received_ >= toReceive_) {
                //std::cout << "Final\n";
                state_ = State::Idle;
                received_ = 0;
                return &message_;
            }
            break;
        }
    }
    return nullptr;
}
} // namespace weather_station
