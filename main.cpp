#include <iostream>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

#include "SevSegShift.h"

int main()
{
    constexpr int SCD4X_I2C_ADDRESS = 0x62;
    stdio_init_all();

    auto baudRate = i2c_init(i2c_default, 100 * 1000);
    std::cout << "Baud " << baudRate << "\n";
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));

    int data = 13;
    int latch = 12;
    int clock = 11;

    auto sendCommand = [](uint16_t command) {
        uint8_t buffer[2];
        buffer[0] = static_cast<uint8_t>((command & 0xFF00) >> 8);
        buffer[1] = static_cast<uint8_t>((command & 0x00FF) >> 0);
        int res = i2c_write_blocking_until(
            i2c_default, SCD4X_I2C_ADDRESS, buffer, 2, true, make_timeout_time_ms(50));
        sleep_ms(1);
        return res;
    };

    SevSegShift sevseg1(data, clock, latch);
    data = 16;
    latch = 17;
    clock = 18;
    SevSegShift sevseg2(data, clock, latch);
    data = 19;
    latch = 20;
    clock = 21;
    SevSegShift sevseg3(data, clock, latch);
    data = 10;
    latch = 9;
    clock = 8;
    SevSegShift sevseg4(data, clock, latch);

    constexpr uint8_t digitPins1[] = {8 + 2, 8 + 5, 8 + 6, 2};
    constexpr uint8_t segmentPins1[] = {8 + 3, 8 + 7, 4, 6, 7, 8 + 4, 3, 5};

    sevseg1.begin(COMMON_CATHODE, 4, digitPins1, segmentPins1, false);
    sevseg2.begin(COMMON_CATHODE, 4, digitPins1, segmentPins1, false);
    sevseg3.begin(COMMON_CATHODE, 4, digitPins1, segmentPins1, false);
    sevseg4.begin(COMMON_CATHODE, 4, digitPins1, segmentPins1, false);
    int res = 0;
    int res1 = sendCommand(0x3F86); // Stop periodic
    sleep_ms(500);
    int res2 = sendCommand(0x3682); // Get Serial

    uint8_t resBuffer[9];
    int res3 = i2c_read_blocking_until(
        i2c_default, SCD4X_I2C_ADDRESS, resBuffer, 9, true, make_timeout_time_ms(50));

    int res4 = sendCommand(0x21B1); // Start measurement
    sleep_ms(5000);
    std::cout << res1 << " " << res2 << " " << res3 << " " << res4 << "\n";
    sevseg1.setNumber(1234);
    sevseg2.setNumber(4567);
    sevseg3.setNumber(-890);

    long last = 0;
    long lastSuccess = 0;

    long val = 0;
    while (true) {
        sevseg1.refreshDisplay();
        sevseg2.refreshDisplay();
        sevseg3.refreshDisplay();
        sevseg4.refreshDisplay();
        auto now = millis();
        if (now - last > 1000) {
            last = now;
            do {
                // Get data ready
                res = sendCommand(0xE4B8);
                if (res != 2) {
                    std::cout << "Error0: " << res << "\n";
                    break;
                }
                //std::cout << "Send data ready: " << res << "\n";

                uint8_t rdybuf[3];
                res = i2c_read_blocking_until(
                    i2c_default, SCD4X_I2C_ADDRESS, rdybuf, 3, true, make_timeout_time_ms(50));
                if (res != 3) {
                    std::cout << "Error1: " << res << "\n";
                    break;
                }
                //std::cout << "Data ready " << res << " buf: " << std::hex << (int)(buf[0]) << " "
                //          << (int)(buf[1]) << " " << (int)(buf[2]) << "\n";

                uint16_t val = (rdybuf[0] << 8) | rdybuf[1];
                //std::cout << std::hex << "Val: " << val << std::endl;
                if ((val & 0x07ff) == 0) {
                    //std::cout << "Not ready" << "\n";
                    break;
                }
                // Read measurement
                res = sendCommand(0xEC05);
                if (res != 2) {
                    std::cout << "Error2: " << res << "\n";
                    break;
                }
                //std::cout << "Send read measurement: " << res << "\n";
                uint8_t buf[9];
                res = i2c_read_blocking_until(
                    i2c_default, SCD4X_I2C_ADDRESS, buf, 9, true, make_timeout_time_ms(50));
                if (res != 9) {
                    std::cout << "Error4: " << res << "\n";
                    break;
                }
                for (int i = 0; i < 9; ++i) {
                    std::cout << std::hex << (int)buf[i] << " ";
                }
                std::cout << "\n";
                uint16_t val1 = (buf[0] << 8) | buf[1];
                uint16_t val2 = (buf[3] << 8) | buf[4];
                uint16_t val3 = (buf[6] << 8) | buf[7];
                std::cout << "Val1: " << val1 << " Val2: " << val2 << " Val3: " << val3 << "\n";

                auto co2 = (int)val1;
                auto temperature = static_cast<float>(val2 * 175.0 / 65535.0 - 45.0);
                auto humidity = static_cast<float>(val3 * 100.0 / 65535.0);
                std::cout << "CO2: " << std::dec << co2 << " Temp: " << temperature
                          << " Humidity: " << humidity << "\n";
                sevseg1.setNumber(co2);
                sevseg2.setNumberF(temperature, 2);
                uint8_t segs[4];
                sevseg2.getSegments(segs);
                segs[3] = 0b01011000;
                sevseg2.setSegments(segs);
                sevseg3.setNumberF(humidity, 2);
                lastSuccess = now;

            } while (0);
        }
        sevseg4.setNumber((now - lastSuccess) / 100);
    }
}
