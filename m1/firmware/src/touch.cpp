#include "touch.h"

#include <Arduino.h>
#include <Wire.h>
#include <hardware/i2c.h>

#include "board.h"

namespace ocfw {
namespace {

constexpr uint8_t kQtChipId = 0x3E;
constexpr int kXyMax = 255;

// IQS572 address-commands. A read is the command, a repeated start, then the bytes.
constexpr uint8_t kCmdVersion = 0x00;
constexpr uint8_t kCmdCounts = 0x04;
constexpr uint8_t kCmdControl = 0x10;
constexpr uint8_t kCmdChannels = 0x15;
constexpr uint8_t kCmdActive = 0x17;
constexpr uint8_t kCtlAckReset = 0x80;
constexpr uint8_t kCtlAutoModes = 0x40;
constexpr uint8_t kCtlAutoAti = 0x04;
constexpr uint8_t kCtlReseed = 0x02;

bool iqs_ok_ = false;
bool qt_ok_ = false;
float pad_x_ = 0.f;
float pad_y_ = 0.f;
bool pad_finger_ = false;
uint8_t strip_ = 0;
bool strip_finger_ = false;
TouchDebug snap_{};

// This IQS572 takes standard I2C: an 8-bit command, then a repeated start for
// a read. STOP closes the window. The Pico SDK read helper disables the
// controller first, and that disable is a STOP, so a command read stays on
// the hardware block and sets the restart bit itself.

bool RdyIsHigh() { return digitalRead(ocboard::kIqsRdy) == HIGH; }

void NoteIqsLevels(uint8_t fingers, int x, int y);

float Axis(int raw) {
    if (raw < 0) raw = 0;
    if (raw > kXyMax) raw = kXyMax;
    return (static_cast<float>(raw) / static_cast<float>(kXyMax)) * 2.f - 1.f;
}

bool QtWrite(uint8_t reg, uint8_t value) {
    Wire1.beginTransmission(ocboard::kQtAddr);
    Wire1.write(reg);
    Wire1.write(value);
    return Wire1.endTransmission() == 0;
}

bool QtRead(uint8_t reg, uint8_t* data, size_t n) {
    Wire1.beginTransmission(ocboard::kQtAddr);
    Wire1.write(reg);
    if (Wire1.endTransmission(false) != 0) return false;
    if (Wire1.requestFrom(ocboard::kQtAddr, n) != n) return false;
    for (size_t i = 0; i < n; ++i) data[i] = static_cast<uint8_t>(Wire1.read());
    return true;
}

void HwRelease() {
    i2c1->restart_on_next = false;
    i2c1->hw->enable = 0;
    (void)i2c1->hw->clr_tx_abrt;
    i2c1->hw->enable = 1;
}

// The command byte is already on the wire and the controller is still enabled.
// The first read command carries the repeated start. The last one carries STOP.
bool HwReadRestart(uint8_t* dst, size_t len, uint32_t timeout_ms) {
    const absolute_time_t until = make_timeout_time_ms(timeout_ms);
    for (size_t i = 0; i < len; ++i) {
        const bool first = i == 0;
        const bool last = i + 1 == len;
        while (!i2c_get_write_available(i2c1)) {
            if (time_reached(until)) {
                HwRelease();
                return false;
            }
        }
        i2c1->hw->data_cmd = (first ? I2C_IC_DATA_CMD_RESTART_BITS : 0) |
                             (last ? I2C_IC_DATA_CMD_STOP_BITS : 0) | I2C_IC_DATA_CMD_CMD_BITS;
        while (!i2c_get_read_available(i2c1)) {
            if (i2c1->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS) {
                (void)i2c1->hw->clr_tx_abrt;
                HwRelease();
                return false;
            }
            if (time_reached(until)) {
                HwRelease();
                return false;
            }
        }
        dst[i] = static_cast<uint8_t>(i2c1->hw->data_cmd);
    }
    i2c1->restart_on_next = false;
    // STOP is queued with the last byte. Wait until the controller finishes
    // it, so the strip's following transaction finds an idle bus.
    while (i2c1->hw->status & I2C_IC_STATUS_ACTIVITY_BITS) {
        if (time_reached(until)) {
            HwRelease();
            return false;
        }
    }
    return true;
}

bool HwWrite(uint8_t cmd, const uint8_t* data, size_t n) {
    uint8_t buf[20];
    if (n + 1 > sizeof(buf)) return false;
    buf[0] = cmd;
    for (size_t i = 0; i < n; ++i) buf[i + 1] = data[i];
    const int ret = i2c_write_blocking_until(i2c1, ocboard::kIqsAddr, buf, n + 1, false,
                                             make_timeout_time_ms(20));
    if (ret != static_cast<int>(n + 1)) {
        HwRelease();
        return false;
    }
    return true;
}

bool HwReadCmd(uint8_t cmd, uint8_t* data, size_t n) {
    const int wr = i2c_write_blocking_until(i2c1, ocboard::kIqsAddr, &cmd, 1, true,
                                            make_timeout_time_ms(20));
    if (wr != 1) {
        HwRelease();
        return false;
    }
    return HwReadRestart(data, n, 20);
}

bool WaitRdy(uint32_t timeout_ms) {
    const uint32_t start = millis();
    while (!RdyIsHigh()) {
        if (millis() - start > timeout_ms) return false;
    }
    return true;
}

// The square grid's four corners have no copper. The pad is a circle.
bool Corner(int tx, int rx) {
    return (tx == 0 || tx == 6) && (rx == 0 || rx == 6);
}

// A finger lowers a cell. The chip's own finger count wanders, so the
// position comes from those low cells. Tx0 is the left edge and Rx0 is the
// top edge. Engine +Y is the top of the pad.
void ApplyCounts() {
    uint16_t ordered[45];
    int n = 0;
    for (int tx = 0; tx < 7; ++tx) {
        for (int rx = 0; rx < 7; ++rx) {
            if (Corner(tx, rx)) continue;
            ordered[n++] = snap_.ch[tx * 7 + rx];
        }
    }
    for (int i = 1; i < n; ++i) {
        const uint16_t v = ordered[i];
        int j = i;
        while (j > 0 && ordered[j - 1] > v) {
            ordered[j] = ordered[j - 1];
            --j;
        }
        ordered[j] = v;
    }
    const int med = ordered[n / 2];

    int weight = 0;
    int tx_w = 0;
    int rx_w = 0;
    for (int tx = 0; tx < 7; ++tx) {
        for (int rx = 0; rx < 7; ++rx) {
            if (Corner(tx, rx)) continue;
            const int v = snap_.ch[tx * 7 + rx];
            if (med <= 20 || v * 2 >= med) continue;
            const int w = med - v;
            weight += w;
            tx_w += tx * w;
            rx_w += rx * w;
        }
    }

    if (weight > 0) {
        const int x = tx_w * 255 / (weight * 6);
        const int y = (weight * 6 - rx_w) * 255 / (weight * 6);
        NoteIqsLevels(1, x, y);
        return;
    }
    snap_.fingers = 0;
    pad_finger_ = false;
}

// RDY must fall, then rise. A later window, not the one a write just used.
bool NextWindow(uint32_t timeout_ms) {
    const uint32_t start = millis();
    while (RdyIsHigh()) {
        if (millis() - start > timeout_ms) return false;
    }
    while (!RdyIsHigh()) {
        if (millis() - start > timeout_ms) return false;
    }
    return true;
}

void ConfigureIqs() {
    if (!WaitRdy(300)) return;
    uint8_t ver[2] = {};
    if (!HwReadCmd(kCmdVersion, ver, sizeof(ver)) || ver[0] != 0x00 || ver[1] != 0x3A) return;

    if (!NextWindow(1500)) return;
    // Rx0–Rx6 and Tx0–Tx6. Rx7, Tx7 and Tx8 are open on this board.
    const uint8_t map[7] = {7, 7, 7, 7, 0x00, 0x00, 0x7F};
    if (!HwWrite(kCmdChannels, map, sizeof(map))) return;

    if (!NextWindow(1500)) return;
    uint8_t active[14] = {};
    for (int tx = 0; tx < 7; ++tx) active[tx * 2 + 1] = 0x7F;
    if (!HwWrite(kCmdActive, active, sizeof(active))) return;

    if (!NextWindow(1500)) return;
    // Ack the reset, run ATI once, keep streaming. Event mode stays off.
    const uint8_t ctl[2] = {static_cast<uint8_t>(kCtlAckReset | kCtlAutoModes | kCtlAutoAti | kCtlReseed),
                            0x00};
    if (!HwWrite(kCmdControl, ctl, sizeof(ctl))) return;

    if (!NextWindow(2500)) return;
    uint8_t back[4] = {};
    if (!HwReadCmd(kCmdChannels, back, sizeof(back))) return;
    if (back[0] != 7 || back[1] != 7 || back[2] != 7 || back[3] != 7) return;
    iqs_ok_ = true;
}

void InitIqs() {
    // NRST is active-low with its own pull-up. It was held low through OLED
    // init so the trackpad could not clock-stretch that bus.
    digitalWrite(ocboard::kIqsNrst, HIGH);
    pinMode(ocboard::kIqsRdy, INPUT);
    delay(20);
    ConfigureIqs();
}

// Pulse/scale stays at the comms default. Scale 6 (divide by 64) was measured
// on this strip: a full swipe only moved a channel by 256 or 512, the touch
// bit never set, and the slider byte stayed 0. The chip's own slider needs
// the unscaled burst.
constexpr uint8_t kPulseScale = 0x00;
constexpr uint8_t kDetectThr = 48;
constexpr uint8_t kDetectInt = 4;

uint8_t rb_fw_ = 0;
uint8_t rb_lp_ = 0;
uint8_t rb_di_ = 0;
uint8_t rb_slider_ = 0;
uint8_t rb_charge_ = 0;
uint8_t rb_thr_ = 0;
uint8_t rb_pulse_ = 0;
uint8_t rb_key0_ = 0;
uint8_t rb_key3_ = 0;

void NoteQtSetup() {
    snap_.slider_rb = rb_slider_;
    snap_.pulse_rb = rb_pulse_;
    snap_.thr_rb = rb_thr_;
}

bool QtSetupMatches() {
    return rb_lp_ == 1 && rb_di_ == kDetectInt && rb_slider_ == 0x80 && rb_charge_ == 10 &&
           rb_thr_ == kDetectThr && rb_pulse_ == kPulseScale && rb_key0_ == 0x00 && rb_key3_ == 0x01;
}

void InitQt() {
    uint8_t id = 0;
    if (!QtRead(0x00, &id, 1) || id != kQtChipId) {
        snap_.qt_ack = false;
        return;
    }
    snap_.qt_ack = true;
    QtRead(0x01, &rb_fw_, 1);

    // KEY3–11 are open pins. Left as sensors they wander and drag the slider
    // reference around. EN=1 takes them out of touch and drives them low.
    // A calibration has to follow any EN change.
    for (uint8_t key = 3; key < 12; ++key) {
        if (!QtWrite(static_cast<uint8_t>(28 + key), 0x01)) return;
    }
    for (uint8_t key = 0; key < 3; ++key) {
        if (!QtWrite(static_cast<uint8_t>(28 + key), 0x00)) return;
        if (!QtWrite(static_cast<uint8_t>(40 + key), kPulseScale)) return;
        if (!QtWrite(static_cast<uint8_t>(16 + key), kDetectThr)) return;
    }
    // Datasheet slider on channels 0–2, not a wheel. Charge time covers the
    // 10 kΩ series resistors.
    if (!QtWrite(8, 1)) return;    // 0 powers the chip down. 1 measures every 16 ms.
    if (!QtWrite(9, 20)) return;   // toward-touch drift
    if (!QtWrite(10, 5)) return;   // away-from-touch drift
    if (!QtWrite(11, kDetectInt)) return;
    if (!QtWrite(12, 255)) return; // touch recal delay
    if (!QtWrite(13, 25)) return;  // drift hold
    if (!QtWrite(15, 10)) return;  // charge time
    if (!QtWrite(14, 0x80)) return;

    uint8_t block[8] = {};
    if (!QtRead(8, block, sizeof(block))) return;
    rb_lp_ = block[0];
    rb_di_ = block[3];
    rb_slider_ = block[6];
    rb_charge_ = block[7];
    if (!QtRead(16, &rb_thr_, 1) || !QtRead(28, &rb_key0_, 1) || !QtRead(31, &rb_key3_, 1) ||
        !QtRead(40, &rb_pulse_, 1)) {
        return;
    }
    NoteQtSetup();
    if (!QtSetupMatches()) return;

    if (!QtWrite(6, 0x01)) return;
    const uint32_t start = millis();
    bool calibrated = false;
    while (millis() - start < 500) {
        uint8_t status = 0;
        if (!QtRead(2, &status, 1)) return;
        if ((status & 0x80) == 0) {
            calibrated = true;
            break;
        }
        delay(16);
    }
    qt_ok_ = calibrated;
}

void NoteIqsLevels(uint8_t fingers, int x, int y) {
    snap_.fingers = fingers;
    snap_.x = x;
    snap_.y = y;
    pad_finger_ = fingers > 0;
    if (pad_finger_) {
        pad_x_ = Axis(x);
        pad_y_ = Axis(y);
    }
}

void ReadIqs() {
    snap_.iqs_ok = iqs_ok_;
    // The play loop must not wait on this pin. A closed window just means
    // try again next pass. Blocking here stalls the keys and the strip,
    // which share this bus.
    static bool seen_high = false;
    if (digitalRead(ocboard::kIqsRdy) == LOW) {
        seen_high = false;
        return;
    }
    if (!iqs_ok_ || seen_high) return;
    seen_high = true;
    uint8_t raw[98];
    if (!HwReadCmd(kCmdCounts, raw, sizeof(raw))) return;
    for (int i = 0; i < 49; ++i) {
        snap_.ch[i] = static_cast<uint16_t>((raw[i * 2] << 8) | raw[i * 2 + 1]);
    }
    snap_.ch_ok = true;
    ApplyCounts();
}

void ReadQtChannels() {
    static uint32_t last_ms = 0;
    if (millis() - last_ms < 16 || Wire1.getTimeoutFlag()) return;
    last_ms = millis();
    uint8_t sig[6];
    uint8_t refv[6];
    if (!QtRead(52, sig, sizeof(sig)) || !QtRead(76, refv, sizeof(refv))) return;
    for (int i = 0; i < 3; ++i) {
        snap_.sig[i] = static_cast<uint16_t>((static_cast<unsigned>(sig[i * 2]) << 8) | sig[i * 2 + 1]);
        snap_.refv[i] =
            static_cast<uint16_t>((static_cast<unsigned>(refv[i * 2]) << 8) | refv[i * 2 + 1]);
    }
}

void ReadQt() {
    snap_.qt_ok = qt_ok_;
    uint8_t raw[4];
    // Address 2: detection status, key status, slider position.
    // Position is valid only while SDET or a slider key is set.
    if (!QtRead(2, raw, sizeof(raw))) {
        snap_.qt_ack = false;
        return;
    }
    snap_.qt_ack = true;
    snap_.status = raw[0];
    snap_.keys = raw[1];
    snap_.chip_pos = raw[3];
    uint8_t sigb[6];
    uint8_t refb[6];
    if (QtRead(52, sigb, sizeof(sigb)) && QtRead(76, refb, sizeof(refb))) {
        for (int i = 0; i < 3; ++i) {
            snap_.sig[i] =
                static_cast<uint16_t>((static_cast<unsigned>(sigb[i * 2]) << 8) | sigb[i * 2 + 1]);
            snap_.refv[i] =
                static_cast<uint16_t>((static_cast<unsigned>(refb[i * 2]) << 8) | refb[i * 2 + 1]);
        }
    }
    const bool calibrating = (raw[0] & 0x80) != 0;
    // The slider byte is only valid while SDET is set. The key bits follow it.
    const bool finger = (raw[0] & 0x02) != 0 || (raw[1] & 0x07) != 0;
    // On the measured swipe the chip byte sat at 67 for the whole left
    // electrode and reached 255 while the middle electrode was still the big
    // one. A channel under a couple thousand counts is the residual the
    // others leave behind. A channel that has climbed carries its electrode:
    // left 0, middle 128, right 255.
    static int held = -1;
    if (!qt_ok_ || calibrating || !finger) {
        held = -1;
        strip_finger_ = false;
        snap_.pos = 0;
        return;
    }
    auto carry = [](uint16_t sig, uint16_t refv) {
        const int delta = static_cast<int>(sig) - static_cast<int>(refv);
        return delta > 2000 ? delta : 0;
    };
    const int w0 = carry(snap_.sig[0], snap_.refv[0]);
    const int w1 = carry(snap_.sig[1], snap_.refv[1]);
    const int w2 = carry(snap_.sig[2], snap_.refv[2]);
    const int sum = w0 + w1 + w2;
    int pos = sum > 0 ? (w1 * 128 + w2 * 255) / sum : held;
    if (pos < 0) {
        strip_finger_ = false;
        snap_.pos = 0;
        return;
    }
    if (pos > 255) pos = 255;
    held = pos;
    strip_ = static_cast<uint8_t>(pos);
    strip_finger_ = true;
    snap_.pos = strip_;
}

TouchDebug Snapshot() { return snap_; }

} // namespace

void InitTouch() {
    InitIqs();
    InitQt();
}

void FillInput(Input& in) {
    in.trackpad_x = pad_x_;
    in.trackpad_y = pad_y_;
    in.trackpad_finger = pad_finger_;
    in.strip = strip_;
    in.strip_finger = strip_finger_;
}

void ReadTouch(Input& in) {
    // One timed-out transfer means a slave held the clock. Skip the rest of
    // this pass so the recovery inside Wire can finish before we touch it again.
    static uint32_t skip_until = 0;
    if (Wire1.getTimeoutFlag()) {
        Wire1.clearTimeoutFlag();
        skip_until = millis() + 50;
    }
    if (static_cast<int32_t>(millis() - skip_until) < 0) {
        FillInput(in);
        return;
    }
    // Read the strip before the pad's long count transfer. Doing the pad
    // first was landing in the middle of a strip sample.
    ReadQt();
    ReadIqs();
    if (Wire1.getTimeoutFlag()) {
        Wire1.clearTimeoutFlag();
        skip_until = millis() + 50;
    }
    FillInput(in);
}

void SampleStripChannels() {
    static uint32_t last_ms = 0;
    if (millis() - last_ms < 100) return;
    if (Wire1.getTimeoutFlag()) return;
    last_ms = millis();
    ReadQtChannels();
}

void CopyTouchDebug(TouchDebug& out) { out = Snapshot(); }

} // namespace ocfw
