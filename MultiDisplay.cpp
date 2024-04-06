#include "MultiDisplay.h"

#include "ino_compat.h"

#include <map>
#include <utility>
#include <algorithm>

namespace
{

constexpr int32_t powersOf10[] = {1,      10,      100,      1000,      10000,
                                  100000, 1000000, 10000000, 100000000, 1000000000};

constexpr int32_t powersOf16[] = {0x1,     0x10,     0x100,     0x1000,
                                  0x10000, 0x100000, 0x1000000, 0x10000000};

std::map<uint8_t, uint8_t> digitSegments = {
    //     GFEDCBA  Segments      7-segment map:
    {0, 0b00111111},   // 0   "0"          AAA
    {1, 0b00000110},   // 1   "1"         F   B
    {2, 0b01011011},   // 2   "2"         F   B
    {3, 0b01001111},   // 3   "3"          GGG
    {4, 0b01100110},   // 4   "4"         E   C
    {5, 0b01101101},   // 5   "5"         E   C
    {6, 0b01111101},   // 6   "6"          DDD
    {7, 0b00000111},   // 7   "7"
    {8, 0b01111111},   // 8   "8"
    {9, 0b01101111},   // 9   "9"
    {'A', 0b01110111}, // 65  'A'
    {'b', 0b01111100}, // 66  'b'
    {'C', 0b00111001}, // 67  'C'
    {'d', 0b01011110}, // 68  'd'
    {'E', 0b01111001}, // 69  'E'
    {'F', 0b01110001}, // 70  'F'
    {'G', 0b00111101}, // 71  'G'
    {'H', 0b01110110}, // 72  'H'
    {'I', 0b00110000}, // 73  'I'
    {'J', 0b00001110}, // 74  'J'
    {'K', 0b01110110}, // 75  'K'  Same as 'H'
    {'L', 0b00111000}, // 76  'L'
    {'M', 0b00000000}, // 77  'M'  NO DISPLAY
    {'n', 0b01010100}, // 78  'n'
    {'O', 0b00111111}, // 79  'O'
    {'P', 0b01110011}, // 80  'P'
    {'q', 0b01100111}, // 81  'q'
    {'r', 0b01010000}, // 82  'r'
    {'S', 0b01101101}, // 83  'S'
    {'t', 0b01111000}, // 84  't'
    {'U', 0b00111110}, // 85  'U'
    {'V', 0b00111110}, // 86  'V'  Same as 'U'
    {'W', 0b00000000}, // 87  'W'  NO DISPLAY
    {'X', 0b01110110}, // 88  'X'  Same as 'H'
    {'y', 0b01101110}, // 89  'y'
    {'Z', 0b01011011}, // 90  'Z'  Same as '2'
    {' ', 0b00000000}, // 32  ' '  BLANK
    {'-', 0b01000000}, // 45  '-'  DASH
    {'.', 0b10000000}, // 46  '.'  PERIOD
    {'*', 0b01100011}, // 42 '*'  DEGREE ..
    {'_', 0b00001000}, // 95 '_'  UNDERSCORE
};
} // namespace

namespace weather_station
{
MultiDisplay::MultiDisplay(
    Pin clock, Pin latch, const std::vector<Pin>& data, std::array<uint8_t, 4>&& digitPins,
    std::array<uint8_t, 8>&& segmentPins
)
    : digitPins_(digitPins)
    , segmentPins_(segmentPins)
    , dataPins_(data)
    , latch_(latch)
    , clock_(clock)
{
    activeSegments_.resize(data.size());
    registerValues_.resize(data.size());
    std::for_each(registerValues_.begin(), registerValues_.end(), [](auto& val) { val = 0; });

    auto pinMode = [](uint8_t pin, bool out) {
        gpio_init(pin);
        gpio_set_dir(pin, out);
    };
    for (auto d : dataPins_) {
        pinMode(d, true);
    }
    pinMode(latch_, true);
    pinMode(clock_, true);
}

void MultiDisplay::setSegments(int idx, const std::array<uint8_t, 4>& numbers, int dotPos)
{
    auto& activeSegments = activeSegments_[idx];
    for (int i = 0; i < 4; ++i) {
        if (digitSegments.count(numbers[i]) == 0) {
            activeSegments[i] = digitSegments['-'];
            continue;
        }
        activeSegments[i] = digitSegments[numbers[i]];
    }
    if (dotPos > 0) {
        activeSegments[dotPos - 1] |= digitSegments['.'];
    }
}

void MultiDisplay::setNumber(int idx, int32_t num, int8_t dotPos, bool hex)
{
    std::array<uint8_t, 4> digits = {' ', ' ', ' ', ' '};
    const auto powersOfBase = hex ? powersOf16 : powersOf10;
    const auto maxNum = powersOfBase[4] - 1;
    const auto minNum = -(powersOfBase[4 - 1] - 1);
    if (num > maxNum || num < minNum) {
        for (uint8_t digitNum = 0; digitNum < 4; digitNum++) {
            digits[digitNum] = '-';
        }
        setSegments(idx, digits);
        return;
    }
    uint8_t digitNum = 0;

    // Convert all number to positive values
    if (num < 0) {
        digits[0] = '-';
        digitNum = 1; // Skip the first iteration
        num = -num;
    }

    // Find all digits for base's representation, starting with the most
    // significant digit
    for (; digitNum < 4; digitNum++) {
        int32_t factor = powersOfBase[4 - 1 - digitNum];
        digits[digitNum] = num / factor;
        num -= digits[digitNum] * factor;
    }

    int eraseZeroesTo = 3;
    if (dotPos > 0) {
        eraseZeroesTo = dotPos - 1;
    }
    for (int i = 0; i < eraseZeroesTo; ++i) {
        if (digits[i] != 0) {
            break;
        }
        digits[i] = ' ';
    }

    setSegments(idx, digits, dotPos);
}

void MultiDisplay::setNumberF(int idx, float num, int8_t decPlaces)
{
    int8_t decPlacesPos = std::clamp(decPlaces, (int8_t)0, (int8_t)4);
    num *= powersOf10[decPlacesPos];
    // Modify the number so that it is rounded to an integer correctly
    num += (num >= 0.f) ? 0.5f : -0.5f;
    setNumber(idx, (int32_t)num, decPlaces, false);
}

void MultiDisplay::setSegment(int idx, int digit, uint8_t segments)
{
    activeSegments_[idx][digit] = segments;
}

void MultiDisplay::refreshDisplay()
{
    auto now = time_us_64();
    if (now - lastElementSwitch_ > switchDelay_) {
        lastElementSwitch_ = now;

        if (mode_ == Mode::Segment) {
            displayedElementIdx_ = (displayedElementIdx_ + 1) % 8;

            for (int i = 0; i < registerValues_.size(); ++i) {
                auto& registerValues = registerValues_[i];
                const auto& activeSegments = activeSegments_[i];
                auto segmentPin = segmentPins_[displayedElementIdx_];
                registerValues = 1 << segmentPin;

                for (int digit = 0; digit < 4; ++digit) {
                    if ((activeSegments[digit] & (1 << displayedElementIdx_)) == 0) {
                        registerValues |= (1 << digitPins_[digit]);
                    }
                }
            }
        } else if (mode_ == Mode::Digit) {
            displayedElementIdx_ = (displayedElementIdx_ + 1) % 4;

            for (int i = 0; i < registerValues_.size(); ++i) {
                auto& registerValues = registerValues_[i];
                const auto& activeSegments = activeSegments_[i];
                registerValues = 0;

                for (int d = 0; d < 4; ++d) {
                    if (d != displayedElementIdx_) {
                        registerValues |= 1 << digitPins_[d];
                    }
                }
                auto segments = activeSegments[displayedElementIdx_];
                for (int s = 0; s < 8; ++s) {
                    if (segments & (1 << s)) {
                        registerValues |= 1 << segmentPins_[s];
                    }
                }
            }
        }
        pushToRegisters();
    }
}

void MultiDisplay::pushToRegisters()
{
    for (int regVal = 15; regVal >= 0; --regVal) {
        for (int display = 0; display < registerValues_.size(); ++display) {
            auto val = registerValues_[display];
            int bit = ((val & (1 << regVal)) == 0) ? 0 : 1;
            gpio_put(dataPins_[display], bit);
        }
        gpio_put(clock_, 0);
        gpio_put(clock_, 1);
    }
    gpio_put(latch_, 0);
    gpio_put(latch_, 1);
}
} // namespace weather_station
