#pragma once

#include <map>
#include <cstdint>
#include <memory>
#include <array>
#include <optional>

namespace weather_station
{

struct Message
{
    enum class Type : uint32_t { Unknown, WeatherInfo, IncDelay, DecDelay };
    Type type = Type::Unknown;
    std::array<uint32_t, 8> data;
};

class Receiver
{
public:
    Message* process();

private:
    enum class State { Idle, Receiving };
    uint64_t idleTimeoutMs_ = 5;
    State state_ = State::Idle;
    Message message_;
    int received_ = 0;
    int toReceive_ = 0;
};
} // namespace weather_station
