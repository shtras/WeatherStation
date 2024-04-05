#pragma once

#include <vector>
#include <array>
#include <stdint.h>

class MultiDisplay
{
public:
    using Pin = uint8_t;
    MultiDisplay(Pin clockPin, Pin latchPin, const std::vector<Pin>& dataPins,
        std::array<uint8_t, 4>&& digitPins, std::array<uint8_t, 8>&& segmentPins);

    void begin();

    void refreshDisplay();

    void setBrightness(int brightness);

    void setNumber(int idx, int32_t numToShow, int8_t decPlaces = -1, bool hex = 0);
    void setNumberF(int idx, float numToShow, int8_t decPlaces = -1, bool hex = 0);

    void setSegments(int idx, const uint8_t segs[]);
    void getSegments(int idx, uint8_t segs[]);
    void setChars(const char str[]);
    void blank(void);

private:
    void segmentOn(uint8_t segmentNum);
    void segmentOff(uint8_t segmentNum);

    void pushToRegisters();

    std::vector<Pin> dataPins_;
    int numDisplays_ = 1;
    Pin clockPin_;
    Pin latchPin_;

    std::array<uint8_t, 4> digitPins_;
    std::array<uint8_t, 8> segmentPins_;

    std::vector<std::array<uint8_t, 4>> digitValues;

    std::vector<std::array<uint8_t, 16>> shiftRegisterData_;

    uint8_t prevUpdateIdx = 0;   // The previously updated segment or digit
    uint32_t prevUpdateTime = 0; // The time (millis()) when the display was last updated
    int16_t ledOnTime = 200;       // The time (us) to wait with LEDs on
    int16_t waitOffTime = 0;     // The time (us) to wait with LEDs off
    bool waitOffActive = false;  // Whether  the program is waiting with LEDs off
};
