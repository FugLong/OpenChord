#include "oled.h"

#include <Arduino.h>
#include <Wire.h>

#include "board.h"

namespace ocfw {
namespace {

constexpr uint8_t kWidth = 128;
constexpr uint8_t kPages = 4;

bool begun_ = false;

bool Command(uint8_t cmd) {
    Wire.beginTransmission(ocboard::kOledAddr);
    Wire.write(0x00);
    Wire.write(cmd);
    return Wire.endTransmission() == 0;
}

} // namespace

bool InitOled() {
    Wire.setSDA(ocboard::kI2cSda);
    Wire.setSCL(ocboard::kI2cScl);
    Wire.begin();
    Wire.setClock(400000);

    // 128×32 SSD1306. COM pins 0x02 is the 32-row panel, not the 64-row one.
    const uint8_t seq[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x1F, 0xD3, 0x00, 0x40, 0x8D, 0x14, 0x20, 0x00,
        0xA1, 0xC8, 0xDA, 0x02, 0x81, 0x8F, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF,
    };
    bool ok = true;
    for (uint8_t cmd : seq) ok = Command(cmd) && ok;
    begun_ = ok;
    return ok;
}

bool OledReady() { return begun_; }

bool PresentOled(const uint8_t bitmap[kScreenBytes]) {
    if (!begun_) return false;
    for (uint8_t page = 0; page < kPages; ++page) {
        if (!Command(static_cast<uint8_t>(0xB0 + page))) return false;
        if (!Command(0x00)) return false;
        if (!Command(0x10)) return false;
        const uint8_t* row = bitmap + page * kWidth;
        for (uint8_t col = 0; col < kWidth; col += 16) {
            Wire.beginTransmission(ocboard::kOledAddr);
            Wire.write(0x40);
            for (uint8_t i = 0; i < 16; ++i) Wire.write(row[col + i]);
            if (Wire.endTransmission() != 0) return false;
        }
    }
    return true;
}

} // namespace ocfw
