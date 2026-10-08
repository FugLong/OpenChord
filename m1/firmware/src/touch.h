#pragma once

#include "input.h"

namespace ocfw {

// IQS572 trackpad and QT2120 strip, on the same I2C bus as the OLED.
// Call after Wire1 is up. A missing chip is skipped; the keys keep working.
void InitTouch();
void ReadTouch(Input& in);
// Signal and reference for the three slider channels. Not part of the play loop.
void SampleStripChannels();

// Live numbers for the debug pages. Updated by ReadTouch.
struct TouchDebug {
    bool qt_ok = false;
    bool qt_ack = false;
    uint8_t status = 0;
    uint8_t keys = 0;
    uint8_t pos = 0;
    uint8_t chip_pos = 0;
    // Read back after setup. Page 0 shows these so a missed write is visible.
    uint8_t slider_rb = 0;
    uint8_t pulse_rb = 0;
    uint8_t thr_rb = 0;
    uint16_t sig[3]{};
    uint16_t refv[3]{};
    bool iqs_ok = false;
    uint8_t fingers = 0;
    int x = 0;
    int y = 0;
    // Count command. Index tx * 7 + rx. On the pad, Tx runs left to right and Rx runs top to bottom.
    bool ch_ok = false;
    uint16_t ch[49]{};
};

void CopyTouchDebug(TouchDebug& out);

} // namespace ocfw
