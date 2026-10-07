#include "touch.h"

#include <Arduino.h>
#include <Wire.h>

#include "board.h"

namespace ocfw {
namespace {

constexpr uint8_t kQtChipId = 0x3E;
constexpr int kXyMax = 255;

bool iqs_ok_ = false;
// Some windows only answer a read that is its own transaction.
bool iqs_read_stop_ = false;
bool qt_ok_ = false;
float pad_x_ = 0.f;
float pad_y_ = 0.f;
bool pad_finger_ = false;
uint8_t strip_ = 0;
bool strip_finger_ = false;
// Loudest positive reading each electrode has shown. The outer tapers are
// one electrode wide, so position along them comes from this.
uint16_t strip_peak_[3] = {1, 1, 1};
bool channels_fresh_ = false;
TouchDebug snap_{};

bool IqsWrite(uint16_t addr, const uint8_t* data, size_t n) {
    Wire1.beginTransmission(ocboard::kIqsAddr);
    Wire1.write(static_cast<uint8_t>(addr >> 8));
    Wire1.write(static_cast<uint8_t>(addr));
    for (size_t i = 0; i < n; ++i) Wire1.write(data[i]);
    return Wire1.endTransmission() == 0;
}

bool IqsRead(uint16_t addr, uint8_t* data, size_t n) {
    Wire1.beginTransmission(ocboard::kIqsAddr);
    Wire1.write(static_cast<uint8_t>(addr >> 8));
    Wire1.write(static_cast<uint8_t>(addr));
    // Repeated-start is the datasheet read. A STOP in between is the fallback
    // when that read just echoes one byte twice.
    if (Wire1.endTransmission(iqs_read_stop_) != 0) return false;
    if (iqs_read_stop_) delayMicroseconds(200);
    if (Wire1.requestFrom(ocboard::kIqsAddr, n) != n) return false;
    for (size_t i = 0; i < n; ++i) data[i] = static_cast<uint8_t>(Wire1.read());
    return true;
}

void IqsEndWindow() {
    const uint8_t any = 0x01;
    IqsWrite(0xEEEE, &any, 1);
}

bool WaitRdy(uint32_t timeout_ms) {
    const uint32_t start = millis();
    while (digitalRead(ocboard::kIqsRdy) == LOW) {
        if (millis() - start >= timeout_ms) return false;
    }
    return true;
}

// Product number is 58 (0x003A). A window that never took the address comes
// back as 0x3A3A, which is still this chip.
bool ProductIs58(uint8_t hi, uint8_t lo) {
    const int be = (static_cast<int>(hi) << 8) | lo;
    const int le = (static_cast<int>(lo) << 8) | hi;
    return be == 58 || le == 58;
}

bool ProductAccept(uint8_t hi, uint8_t lo) {
    return ProductIs58(hi, lo) || hi == 0x3A || lo == 0x3A;
}

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

// Read the product bytes and close the window. `stop` splits the address
// write from the read; the datasheet path keeps them on one repeated start.
bool ReadProduct(bool stop, uint8_t id[2]) {
    if (!WaitRdy(200)) {
        snap_.iqs_fail = IqsFail::NoRdy;
        return false;
    }
    const bool saved = iqs_read_stop_;
    iqs_read_stop_ = stop;
    const bool ok = IqsRead(0x0000, id, 2);
    iqs_read_stop_ = saved;
    IqsEndWindow();
    if (!ok) snap_.iqs_fail = IqsFail::Nack;
    return ok;
}

void ConfigureIqs() {
    // The ID read used up that window. Settings have to land in the next one
    // or the chip ignores them and the pad stays at the factory grid.
    if (!WaitRdy(200)) {
        snap_.iqs_fail = IqsFail::NoRdy;
        return;
    }

    // Rev A electrodes are Rx0–6 and Tx0–6. Factory memory is a different grid.
    // Rx mapping is 10 bytes at 0x063F; Tx mapping is 15 bytes at 0x0649.
    const uint8_t nrx = 7;
    const uint8_t ntx = 7;
    // Unused map slots must not repeat Rx0/Tx0 or the tune treats them as
    // extra copies of the first pin and gives up.
    const uint8_t rxmap[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    const uint8_t txmap[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    const uint8_t res[2] = {0x01, 0x00};
    IqsWrite(0x063D, &nrx, 1);
    IqsWrite(0x063E, &ntx, 1);
    IqsWrite(0x063F, rxmap, 10);
    IqsWrite(0x0649, txmap, 15);
    IqsWrite(0x066E, res, 2);
    IqsWrite(0x0670, res, 2);
    // 15 Tx rows, 2 bytes each. Low 7 bits of each used row are Rx0–6.
    uint8_t active[30] = {};
    for (int tx = 0; tx < 7; ++tx) active[tx * 2] = 0x7F;
    IqsWrite(0x067B, active, 15);
    IqsWrite(0x068A, active + 15, 15);
    const uint8_t alp_off = 0;
    IqsWrite(0x0658, &alp_off, 1);

    // Streaming, not event mode. SETUP_COMPLETE is what lets the chip leave
    // configuration and tune. AUTO_ATI without it sets ATI_ERROR and the
    // finger count stays 0 no matter where you touch.
    const uint8_t cfg0 = 0x60;             // SETUP_COMPLETE | WDT
    const uint8_t cfg1 = 0x00;             // clear EVENT_MODE
    const uint8_t ctrl = 0x80 | 0x20 | 0x08; // ACK_RESET | AUTO_ATI | RESEED
    IqsWrite(0x058E, &cfg0, 1);
    IqsWrite(0x058F, &cfg1, 1);
    IqsWrite(0x0431, &ctrl, 1);
    IqsEndWindow();

    for (int i = 0; i < 20; ++i) {
        delay(40);
        if (!WaitRdy(40)) continue;
        uint8_t raw[2];
        if (IqsRead(0x000F, raw, 2)) snap_.info = raw[0];
        IqsEndWindow();
        // SHOW_RESET, ALP_ATI_ERROR, ATI_ERROR.
        if ((snap_.info & 0xA8) == 0) break;
    }
    snap_.iqs_fail = IqsFail::Ok;
    iqs_ok_ = true;
}

void InitIqs() {
    // NRST is active-low with its own pull-up. It was held low through OLED
    // init so the trackpad could not clock-stretch that bus.
    digitalWrite(ocboard::kIqsNrst, HIGH);
    pinMode(ocboard::kIqsRdy, INPUT);
    delay(10);

    uint8_t id[2] = {};
    if (!ReadProduct(false, id)) return;
    if (!ProductIs58(id[0], id[1])) {
        uint8_t alt[2] = {};
        if (ReadProduct(true, alt) && ProductIs58(alt[0], alt[1])) {
            id[0] = alt[0];
            id[1] = alt[1];
            iqs_read_stop_ = true;
        }
    }
    snap_.product = static_cast<uint16_t>((static_cast<int>(id[0]) << 8) | id[1]);
    if (!ProductAccept(id[0], id[1])) {
        snap_.iqs_fail = IqsFail::BadId;
        return;
    }
    ConfigureIqs();
}

void InitQt() {
    uint8_t id = 0;
    if (!QtRead(0x00, &id, 1) || id != kQtChipId) {
        snap_.qt_ack = false;
        return;
    }
    snap_.qt_ack = true;

    // KEY3–11 are open pins. Left as sensors they wander and drag the slider
    // reference around. EN=1 takes them out of touch and drives them low.
    for (uint8_t key = 3; key < 12; ++key) {
        if (!QtWrite(static_cast<uint8_t>(28 + key), 0x01)) return;
    }
    for (uint8_t key = 0; key < 3; ++key) {
        if (!QtWrite(static_cast<uint8_t>(28 + key), 0x00)) return;
        if (!QtWrite(static_cast<uint8_t>(40 + key), 0x00)) return;
        // A threshold of 10 let idle noise freeze drift and leave a stuck delta.
        if (!QtWrite(static_cast<uint8_t>(16 + key), 48)) return;
    }
    // Datasheet power-on slider, plus a longer charge pulse for the 10 kΩ
    // series resistors. KEY0–2 are one 0–255 slider, not a wheel.
    if (!QtWrite(8, 1)) return;    // LP: measure every 16 ms
    if (!QtWrite(9, 20)) return;   // toward-touch drift
    if (!QtWrite(10, 5)) return;   // away-from-touch drift
    if (!QtWrite(11, 4)) return;   // detect integrator
    if (!QtWrite(12, 255)) return; // touch recal delay
    if (!QtWrite(13, 25)) return;  // drift hold
    if (!QtWrite(15, 10)) return;  // charge time
    if (!QtWrite(14, 0x80)) return;
    if (!QtWrite(6, 0x01)) return;

    const uint32_t start = millis();
    while (millis() - start < 400) {
        uint8_t status = 0;
        if (!QtRead(2, &status, 1)) return;
        if ((status & 0x80) == 0) break;
        delay(16);
    }
    qt_ok_ = true;
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

void ProbeIqs() {
    static uint32_t last_ms = 0;
    if (millis() - last_ms < 200) return;
    last_ms = millis();
    uint8_t id[2];
    if (!IqsRead(0x0000, id, 2)) {
        IqsEndWindow();
        snap_.iqs_fail = IqsFail::Nack;
        return;
    }
    IqsEndWindow();
    snap_.product = static_cast<uint16_t>((static_cast<int>(id[0]) << 8) | id[1]);
    if (!ProductAccept(id[0], id[1])) {
        snap_.iqs_fail = IqsFail::BadId;
        return;
    }
    ConfigureIqs();
}

void ReadIqs() {
    snap_.iqs_ok = iqs_ok_;
    // RDY stays high only while a window is open, then drops if nobody answers.
    // Waiting for the next rising edge is the start of that window, not a
    // sample of whatever the OLED loop happened to catch.
    if (!WaitRdy(15)) {
        snap_.rdy = false;
        return;
    }
    snap_.rdy = true;
    if (!iqs_ok_) {
        ProbeIqs();
        return;
    }
    // One read for the whole block: info, a spare, fingers, rel XY, abs XY.
    uint8_t raw[11];
    if (!IqsRead(0x000F, raw, sizeof(raw))) {
        IqsEndWindow();
        snap_.iqs_fail = IqsFail::Nack;
        return;
    }
    snap_.info = raw[0];
    const int fingers = raw[2];
    const int x = (static_cast<int>(raw[7]) << 8) | raw[8];
    const int y = (static_cast<int>(raw[9]) << 8) | raw[10];
    // A stuck window repeats 0x3A in every byte. That is not five fingers.
    if (fingers > 5) {
        snap_.fingers = static_cast<uint8_t>(fingers);
        snap_.x = x;
        snap_.y = y;
        pad_finger_ = false;
        IqsEndWindow();
        return;
    }
    NoteIqsLevels(static_cast<uint8_t>(fingers), x, y);
    snap_.iqs_fail = IqsFail::Ok;
    IqsEndWindow();
}

// Sense copper runs x = -27.5 .. +27.5 mm. Grounded chevrons sit outside
// that. Each electrode is widest at -15, 0, and +15 mm. Past the wide
// point the count falls through the reference and the sign goes negative.
constexpr int kSenseL = -275;
constexpr int kSenseR = 275;
constexpr int kSenseSpan = kSenseR - kSenseL;
constexpr int kPeakX[3] = {-150, 0, 150};
// Counts below this are noise on every electrode.
constexpr int kDead = 280;
// A new touch has to clear this. A swipe already in progress may fall back to kDead.
constexpr int kOn = 500;

int StripPosFromX(int x_tenth_mm) {
    if (x_tenth_mm < kSenseL) x_tenth_mm = kSenseL;
    if (x_tenth_mm > kSenseR) x_tenth_mm = kSenseR;
    return (x_tenth_mm - kSenseL) * 255 / kSenseSpan;
}

void ReleaseStripFinger() {
    strip_finger_ = false;
    snap_.pos = 0;
    static uint32_t decay_ms = 0;
    if (millis() - decay_ms < 80) return;
    decay_ms = millis();
    for (int i = 0; i < 3; ++i) {
        if (strip_peak_[i] > 32) strip_peak_[i] = static_cast<uint16_t>(strip_peak_[i] * 7 / 8);
    }
}

void ApplyStripPosition() {
    if (!channels_fresh_) return;

    int s[3];
    int p[3];
    int sum = 0;
    for (int i = 0; i < 3; ++i) {
        s[i] = static_cast<int>(snap_.sig[i]) - static_cast<int>(snap_.refv[i]);
        p[i] = s[i] > kDead ? s[i] : 0;
        sum += p[i];
        if (p[i] >= kOn && p[i] > strip_peak_[i]) strip_peak_[i] = static_cast<uint16_t>(p[i]);
    }

    static int track = 0;
    static bool tracking = false;
    static uint8_t quiet = 0;
    static uint8_t arm = 0;

    bool pressed = false;
    for (int i = 0; i < 3; ++i)
        if (p[i] >= kOn) pressed = true;
    // A negative drift is not a finger. It only keeps a touch that is already
    // out on a tip, and only when it is large next to that electrode's peak.
    if (tracking && !pressed) {
        for (int i = 0; i < 3 && !pressed; ++i) {
            if (strip_peak_[i] > kOn && p[i] > strip_peak_[i] / 6) pressed = true;
        }
        if (track <= kPeakX[0] && strip_peak_[0] > kOn && -s[0] > strip_peak_[0] / 5)
            pressed = true;
        if (track >= kPeakX[2] && strip_peak_[2] > kOn && -s[2] > strip_peak_[2] / 5)
            pressed = true;
    }
    if (!tracking) {
        if (pressed) {
            if (arm < 2) ++arm;
            pressed = arm >= 2;
        } else {
            arm = 0;
        }
    } else {
        arm = 0;
    }

    const bool calibrating = (snap_.status & 0x80) != 0;
    if (!qt_ok_ || calibrating || !pressed) {
        // One quiet sample is the count crossing the reference, not a lift.
        // Releasing on that frame snaps the position back to the left end.
        if (tracking && quiet < 4) {
            ++quiet;
            return;
        }
        tracking = false;
        quiet = 0;
        ReleaseStripFinger();
        return;
    }
    quiet = 0;

    // The grounded chevron at each tip sits past the copper. The end
    // electrode falls back to nothing there, and a little of the center
    // electrode is still positive. A centroid of that leftover is the
    // middle of the strip, so an end the finger is already on stays an end.
    const int mid_peak = strip_peak_[1] > 32 ? strip_peak_[1] : 32;
    const bool mid_real = p[1] > mid_peak / 3;
    const bool outer_left = tracking && track <= kPeakX[0] && p[2] == 0 && !mid_real;
    const bool outer_right = tracking && track >= kPeakX[2] && p[0] == 0 && !mid_real;

    int cx;
    if (outer_left) {
        if (p[0] == 0 || s[0] < -8) {
            cx = kSenseL;
        } else {
            const int peak = strip_peak_[0] > 0 ? strip_peak_[0] : 1;
            int fallen = peak - p[0];
            if (fallen < 0) fallen = 0;
            cx = kPeakX[0] - fallen * (kPeakX[0] - kSenseL) / peak;
            if (cx < kSenseL) cx = kSenseL;
        }
    } else if (outer_right) {
        if (p[2] == 0 || s[2] < -8) {
            cx = kSenseR;
        } else {
            const int peak = strip_peak_[2] > 0 ? strip_peak_[2] : 1;
            int fallen = peak - p[2];
            if (fallen < 0) fallen = 0;
            cx = kPeakX[2] + fallen * (kSenseR - kPeakX[2]) / peak;
            if (cx > kSenseR) cx = kSenseR;
        }
    } else if (p[0] > 0 && p[1] == 0 && p[2] == 0 && (!tracking || track <= kPeakX[0])) {
        const int peak = strip_peak_[0] > 0 ? strip_peak_[0] : 1;
        int fallen = peak - p[0];
        if (fallen < 0) fallen = 0;
        cx = kPeakX[0] - fallen * (kPeakX[0] - kSenseL) / peak;
        if (cx < kSenseL) cx = kSenseL;
    } else if (p[2] > 0 && p[0] == 0 && p[1] == 0 && (!tracking || track >= kPeakX[2])) {
        const int peak = strip_peak_[2] > 0 ? strip_peak_[2] : 1;
        int fallen = peak - p[2];
        if (fallen < 0) fallen = 0;
        cx = kPeakX[2] + fallen * (kSenseR - kPeakX[2]) / peak;
        if (cx > kSenseR) cx = kSenseR;
    } else if (sum > 0) {
        cx = (p[0] * kPeakX[0] + p[2] * kPeakX[2]) / sum;
    } else {
        cx = tracking ? track : 0;
    }

    if (!tracking) {
        track = cx;
        tracking = true;
    } else {
        int step = cx - track;
        // ~8 mm per 16 ms sample. A fast swipe still crosses the strip.
        // A one-frame jump to the other side does not.
        if (step > 80) step = 80;
        if (step < -80) step = -80;
        track += step;
    }

    strip_ = static_cast<uint8_t>(StripPosFromX(track));
    strip_finger_ = true;
    snap_.pos = strip_;
}

void ReadQtChannels() {
    static uint32_t last_ms = 0;
    channels_fresh_ = false;
    if (millis() - last_ms >= 16 && !Wire1.getTimeoutFlag()) {
        last_ms = millis();
        uint8_t sig[6];
        uint8_t refv[6];
        if (QtRead(52, sig, sizeof(sig)) && QtRead(76, refv, sizeof(refv))) {
            for (int i = 0; i < 3; ++i) {
                snap_.sig[i] =
                    static_cast<uint16_t>((static_cast<unsigned>(sig[i * 2]) << 8) | sig[i * 2 + 1]);
                snap_.refv[i] = static_cast<uint16_t>(
                    (static_cast<unsigned>(refv[i * 2]) << 8) | refv[i * 2 + 1]);
            }
            channels_fresh_ = true;
        }
    }
    ApplyStripPosition();
}

void ReadQt() {
    snap_.qt_ok = qt_ok_;
    uint8_t raw[4];
    // Address 2 is status, then the two key bytes, then the chip's own
    // slider byte. That byte only moves where two electrodes overlap.
    if (!QtRead(2, raw, sizeof(raw))) {
        snap_.qt_ack = false;
        return;
    }
    snap_.qt_ack = true;
    snap_.status = raw[0];
    snap_.keys = raw[1];
    ReadQtChannels();
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

void LogTouch() {
    // Only while a terminal is actually reading. A full USB buffer would
    // stall the loop the same way a stuck I2C transaction did.
    if (!Serial.dtr() || Serial.availableForWrite() < 80) return;
    static uint32_t last_ms = 0;
    if (millis() - last_ms < 200) return;
    last_ms = millis();
    Serial.printf("pad inf %02X f %u x %d y %d | qt %u %u %u\n", snap_.info, snap_.fingers, snap_.x,
                  snap_.y, snap_.sig[0], snap_.sig[1], snap_.sig[2]);
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
    ReadIqs();
    ReadQt();
    if (Wire1.getTimeoutFlag()) {
        Wire1.clearTimeoutFlag();
        skip_until = millis() + 50;
    }
    FillInput(in);
    LogTouch();
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
