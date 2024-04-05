#include "MultiDisplay.h"

#include <utility>
#include <algorithm>

#include "pico/stdlib.h"

namespace
{

constexpr int32_t powersOf10[] = {1,                                         // 10^0
    10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000}; // 10^9

constexpr int32_t powersOf16[] = {0x1,                              // 16^0
    0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x10000000}; // 16^7

constexpr int digitCodeMap[] = {
    //     GFEDCBA  Segments      7-segment map:
    0b00111111, // 0   "0"          AAA
    0b00000110, // 1   "1"         F   B
    0b01011011, // 2   "2"         F   B
    0b01001111, // 3   "3"          GGG
    0b01100110, // 4   "4"         E   C
    0b01101101, // 5   "5"         E   C
    0b01111101, // 6   "6"          DDD
    0b00000111, // 7   "7"
    0b01111111, // 8   "8"
    0b01101111, // 9   "9"
    0b01110111, // 65  'A'
    0b01111100, // 66  'b'
    0b00111001, // 67  'C'
    0b01011110, // 68  'd'
    0b01111001, // 69  'E'
    0b01110001, // 70  'F'
    0b00111101, // 71  'G'
    0b01110110, // 72  'H'
    0b00110000, // 73  'I'
    0b00001110, // 74  'J'
    0b01110110, // 75  'K'  Same as 'H'
    0b00111000, // 76  'L'
    0b00000000, // 77  'M'  NO DISPLAY
    0b01010100, // 78  'n'
    0b00111111, // 79  'O'
    0b01110011, // 80  'P'
    0b01100111, // 81  'q'
    0b01010000, // 82  'r'
    0b01101101, // 83  'S'
    0b01111000, // 84  't'
    0b00111110, // 85  'U'
    0b00111110, // 86  'V'  Same as 'U'
    0b00000000, // 87  'W'  NO DISPLAY
    0b01110110, // 88  'X'  Same as 'H'
    0b01101110, // 89  'y'
    0b01011011, // 90  'Z'  Same as '2'
    0b00000000, // 32  ' '  BLANK
    0b01000000, // 45  '-'  DASH
    0b10000000, // 46  '.'  PERIOD
    0b01100011, // 42 '*'  DEGREE ..
    0b00001000, // 95 '_'  UNDERSCORE
};

constexpr int DASH_IDX = 37;
constexpr int BLANK_IDX = 36;
constexpr int PERIOD_IDX = 38;
} // namespace

MultiDisplay::MultiDisplay(Pin clockPin, Pin latchPin, const std::vector<Pin>& dataPins,
    std::array<uint8_t, 4>&& digitPins, std::array<uint8_t, 8>&& segmentPins)
    : dataPins_(dataPins)
    , digitPins_(std::move(digitPins))
    , segmentPins_(std::move(segmentPins))
    , clockPin_(clockPin)
    , latchPin_(latchPin)
{
    digitValues.resize(dataPins.size());
    shiftRegisterData_.resize(dataPins.size());
}

void MultiDisplay::begin()
{
    auto pinMode = [](uint8_t pin, bool out) {
        gpio_init(pin);
        gpio_set_dir(pin, out);
    };
    for (auto d : dataPins_) {
        pinMode(d, true);
    }
    pinMode(latchPin_, true);
    pinMode(clockPin_, true);
    /*
    digitOnVal = LOW;
    segmentOnVal = HIGH;
    */
    blank();
}

void MultiDisplay::refreshDisplay()
{
    auto us = time_us_64();

    // Exit if it's not time for the next display change
    if (waitOffActive) {
        if ((int32_t)(us - prevUpdateTime) < waitOffTime)
            return;
    } else {
        if ((int32_t)(us - prevUpdateTime) < ledOnTime)
            return;
    }
    prevUpdateTime = us;

    /**********************************************/
    // RESISTORS ON DIGITS, UPDATE WITHOUT DELAYS

    if (waitOffActive) {
        waitOffActive = false;
    } else {
        // Turn all lights off for the previous segment
        segmentOff(prevUpdateIdx);

        if (waitOffTime) {
            // Wait a delay with all lights off
            waitOffActive = true;
            return;
        }
    }

    prevUpdateIdx++;
    if (prevUpdateIdx >= 8) {
        prevUpdateIdx = 0;
    }

    // Illuminate the required digits for the new segment
    segmentOn(prevUpdateIdx);
}

void MultiDisplay::setBrightness(int brightness)
{
    auto map = [](long x, long in_min, long in_max, long out_min, long out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    };

    brightness = std::clamp(brightness, -200, 200);
    if (brightness > 0) {
        ledOnTime = map(brightness, 0, 100, 1, 2000);
        waitOffTime = 0;
        waitOffActive = false;
    } else {
        ledOnTime = 0;
        waitOffTime = map(brightness, 0, -100, 1, 2000);
    }
}

void MultiDisplay::setNumber(int idx, int32_t numToShow, int8_t decPlaces, bool hex)
{
    uint8_t digits[4];

    const int32_t* powersOfBase = hex ? powersOf16 : powersOf10;
    const int32_t maxNum = powersOfBase[4] - 1;
    const int32_t minNum = -(powersOfBase[4 - 1] - 1);

    // If the number is out of range, just display dashes
    if (numToShow > maxNum || numToShow < minNum) {
        for (uint8_t digitNum = 0; digitNum < 4; digitNum++) {
            digits[digitNum] = DASH_IDX;
        }
    } else {
        uint8_t digitNum = 0;

        // Convert all number to positive values
        if (numToShow < 0) {
            digits[0] = DASH_IDX;
            digitNum = 1; // Skip the first iteration
            numToShow = -numToShow;
        }

        // Find all digits for base's representation, starting with the most
        // significant digit
        for (; digitNum < 4; digitNum++) {
            int32_t factor = powersOfBase[4 - 1 - digitNum];
            digits[digitNum] = numToShow / factor;
            numToShow -= digits[digitNum] * factor;
        }

        // Find unnnecessary leading zeros and set them to BLANK
        int num = decPlaces;
        if (num < 0)
            num = 0;
        for (digitNum = 0; digitNum < (4 - 1 - num); digitNum++) {
            if (digits[digitNum] == 0) {
                digits[digitNum] = BLANK_IDX;
            }
            // Exit once the first non-zero number is encountered
            else if (digits[digitNum] <= 9) {
                break;
            }
        }
    }

    for (uint8_t digitNum = 0; digitNum < 4; digitNum++) {
        digitValues[idx][digitNum] = digitCodeMap[digits[digitNum]];
        // Set the decimal point segment
        if (decPlaces >= 0) {
            if (digitNum == 4 - 1 - decPlaces) {
                digitValues[idx][digitNum] |= digitCodeMap[PERIOD_IDX];
            }
        }
    }
}

void MultiDisplay::setNumberF(int idx, float numToShow, int8_t decPlaces, bool hex)
{
    int8_t decPlacesPos = std::clamp(decPlaces, (int8_t)0, (int8_t)4);
    if (hex) {
        numToShow = numToShow * powersOf16[decPlacesPos];
    } else {
        numToShow = numToShow * powersOf10[decPlacesPos];
    }
    // Modify the number so that it is rounded to an integer correctly
    numToShow += (numToShow >= 0.f) ? 0.5f : -0.5f;
    setNumber(idx, (int32_t)numToShow, (int8_t)decPlaces, hex);
}

void MultiDisplay::setSegments(int idx, const uint8_t segs[])
{
    for (uint8_t digit = 0; digit < 4; digit++) {
        digitValues[idx][digit] = segs[digit];
    }
}

void MultiDisplay::getSegments(int idx, uint8_t segs[])
{
    for (uint8_t digit = 0; digit < 4; digit++) {
        segs[digit] = digitValues[idx][digit];
    }
}

void MultiDisplay::setChars(const char str[])
{
}

void MultiDisplay::blank(void)
{
    for (uint8_t digitNum = 0; digitNum < 4; digitNum++) {
        for (auto& digitCodesArr : digitValues) {
            digitCodesArr[digitNum] = digitCodeMap[BLANK_IDX];
        }
    }
    segmentOff(0);
}

void MultiDisplay::segmentOn(uint8_t segmentNum)
{
    for (int i = 0; i < shiftRegisterData_.size(); ++i) {
        auto& _shiftRegisterMap = shiftRegisterData_[i];
        _shiftRegisterMap[segmentPins_[segmentNum]] = 1;
        for (uint8_t digitNum = 0; digitNum < 4; digitNum++) {
            if (digitValues[i][digitNum] & (1 << segmentNum)) {
                _shiftRegisterMap[digitPins_[digitNum]] = 0;
            }
        }
    }

    pushToRegisters();
}

void MultiDisplay::segmentOff(uint8_t segmentNum)
{
    for (int i = 0; i < shiftRegisterData_.size(); ++i) {
        auto& _shiftRegisterMap = shiftRegisterData_[i];
        for (uint8_t digitNum = 0; digitNum < 4; digitNum++) {
            _shiftRegisterMap[digitPins_[digitNum]] = 1;
        }
        _shiftRegisterMap[segmentPins_[segmentNum]] = 0;
    }
    pushToRegisters();
}

void MultiDisplay::pushToRegisters()
{
    for (int i = 15; i >= 0; i--) {
        // put the data to the data PIN of Arduino
        for (int j = 0; j < shiftRegisterData_.size(); ++j) {
            auto& _shiftRegisterMap = shiftRegisterData_[j];
            gpio_put(dataPins_[j], _shiftRegisterMap[i]);
        }
        // push it to the next Register (DS -> 0 | 0->1 | 1->2 | ...)
        // this is done by createing a raising flank on SH_CP (ShiftPIN)
        gpio_put(clockPin_, 0);
        gpio_put(clockPin_, 1);
    }
    gpio_put(latchPin_, 0);
    gpio_put(latchPin_, 1);
}
