#pragma once

#include <array>
#include <vector>
#include <cstdint>

namespace weather_station
{

class MultiDisplay
{
public:
    using Pin = uint8_t;
    enum class Mode { Segment, Digit };
    MultiDisplay(
        Pin clock, Pin latch, const std::vector<Pin>& data, std::array<uint8_t, 4>&& digitPins,
        std::array<uint8_t, 8>&& segmentPins
    );
    void setNumber(int idx, int32_t num, int8_t dotPos = -1, bool hex = false);
    void setNumberF(int idx, float num, int8_t decPlaces);
    void setSegment(int idx, int digit, uint8_t segments);
    void refreshDisplay();

    void incDelay()
    {
        idleDelay_ += 50;
        if (idleDelay_ >= switchDelay_) {
            idleDelay_ = switchDelay_ - 50;
        }
    }
    void decDelay()
    {
        idleDelay_ -= 50;
        if (idleDelay_ < 0) {
            idleDelay_ = 0;
        }
    }
    void switchMode()
    {
        if (mode_ == Mode::Segment) {
            mode_ = Mode::Digit;
            setNumber(3, 1);
        } else {
            mode_ = Mode::Segment;
            setNumber(3, 2);
        }
        displayedElementIdx_ = 0;
    }

private:
    void pushToRegisters();
    void setSegments(int idx, const std::array<uint8_t, 4>& numbers, int dotPos = -1);

    std::array<uint8_t, 4> digitPins_;
    std::array<uint8_t, 8> segmentPins_;
    const std::vector<Pin> dataPins_;
    const Pin latch_;
    const Pin clock_;

    std::vector<std::array<uint8_t, 4>> activeSegments_;
    std::vector<uint16_t> registerValues_;

    int displayedElementIdx_ = 0;
    uint64_t lastElementSwitch_ = 0;
    int switchDelay_ = 800;
    int idleDelay_ = 400;
    Mode mode_ = Mode::Segment;
};

} // namespace weather_station
