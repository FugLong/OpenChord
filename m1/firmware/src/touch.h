#pragma once

#include "input.h"

namespace ocfw {

// IQS572 trackpad and QT2120 strip, on the same I2C bus as the OLED.
// Call after Wire1 is up. A missing chip is skipped; the keys keep working.
void InitTouch();
void ReadTouch(Input& in);
// Signal and reference for the three slider channels. Not part of the play loop.
void SampleStripChannels();

// Why the trackpad init stopped. 0 means it came up.
enum class IqsFail : uint8_t { Ok = 0, NoRdy, Nack, BadId };

// Live numbers for the debug pages. Updated by ReadTouch.
struct TouchDebug {
    bool qt_ok = false;
    bool qt_ack = false;
    uint8_t status = 0;
    uint8_t keys = 0;
    uint8_t pos = 0;
    uint16_t sig[3]{};
    uint16_t refv[3]{};
    bool iqs_ok = false;
    IqsFail iqs_fail = IqsFail::NoRdy;
    bool rdy = false;
    uint16_t product = 0;
    uint8_t fingers = 0;
    int x = 0;
    int y = 0;
    uint8_t info = 0;
};

void CopyTouchDebug(TouchDebug& out);

} // namespace ocfw
