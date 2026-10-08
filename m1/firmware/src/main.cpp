#include <Arduino.h>

#include <cstdio>
#include <cstring>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

#include "board.h"
#include "input.h"
#include "oled.h"
#include "screen.h"
#include "session.h"
#include "switches.h"
#include "touch.h"
#include "trs.h"

// Rev A image. Do not flash this onto an RP2040-Zero: the pins are the product board.

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

static oc::Session session;
static ocfw::SurfaceFeed feed;

static void FlushMidi() {
    oc::MidiEvent ev[32];
    const int n = session.drain(ev, 32);
    for (int i = 0; i < n; ++i) {
        switch (ev[i].kind) {
            case oc::MidiEvent::Kind::NoteOn:
                MIDI.sendNoteOn(ev[i].data1, ev[i].data2, ev[i].channel);
                break;
            case oc::MidiEvent::Kind::NoteOff:
                MIDI.sendNoteOff(ev[i].data1, ev[i].data2, ev[i].channel);
                break;
            case oc::MidiEvent::Kind::Cc:
                MIDI.sendControlChange(ev[i].data1, ev[i].data2, ev[i].channel);
                break;
        }
        ocfw::SendTrs(ev[i]);
    }
}

static void OnNoteOn(byte channel, byte pitch, byte velocity) {
    session.noteOn(channel, pitch, velocity);
    FlushMidi();
}

static void OnNoteOff(byte channel, byte pitch, byte velocity) {
    (void)velocity;
    session.noteOff(channel, pitch);
    FlushMidi();
}

static void Show(const uint8_t bitmap[ocfw::kScreenBytes]) {
    static uint8_t shown[ocfw::kScreenBytes];
    static bool have = false;
    if (have && std::memcmp(shown, bitmap, sizeof(shown)) == 0 && ocfw::OledReady()) return;
    if (ocfw::PresentOled(bitmap)) {
        std::memcpy(shown, bitmap, sizeof(shown));
        have = true;
    } else if (!ocfw::OledReady()) {
        ocfw::InitOled();
    }
}

static void Paint() {
    ocfw::ScreenText text;
    session.fillScreen(text.top, sizeof(text.top), text.left, sizeof(text.left), text.mid,
                       sizeof(text.mid), text.right, sizeof(text.right), &text.zones, &text.zone);
    uint8_t bitmap[ocfw::kScreenBytes];
    ocfw::drawScreen(text, bitmap);
    Show(bitmap);
}

// Prev/Next walk Keys, Scale, Drums, then these. Menu still opens settings.
constexpr int kDebugPages = 3;
static int debug_page = -1;

static void PaintDebug() {
    ocfw::TouchDebug d;
    ocfw::CopyTouchDebug(d);
    char row[4][22];
    for (int i = 0; i < 4; ++i) row[i][0] = 0;
    if (debug_page == 0) {
        std::snprintf(row[0], sizeof(row[0]), "Strip");
        if (!d.qt_ack) {
            std::snprintf(row[1], sizeof(row[1]), "no i2c");
        } else {
            const char flags[5] = {
                static_cast<char>((d.status & 0x80) ? 'C' : '-'),
                static_cast<char>((d.status & 0x40) ? 'V' : '-'),
                static_cast<char>((d.status & 0x02) ? 'S' : '-'),
                static_cast<char>((d.status & 0x01) ? 'T' : '-'),
                0,
            };
            std::snprintf(row[1], sizeof(row[1]), "%s pos %u", flags, d.pos);
            std::snprintf(row[2], sizeof(row[2]), "keys %02X c %u", d.keys, d.chip_pos);
            std::snprintf(row[3], sizeof(row[3]), "%s %02X %02X %u", d.qt_ok ? "on" : "fail",
                          d.slider_rb, d.pulse_rb, d.thr_rb);
        }
    } else if (debug_page == 1) {
        std::snprintf(row[0], sizeof(row[0]), "Strip ch");
        for (int i = 0; i < 3; ++i) {
            const int delta = static_cast<int>(d.sig[i]) - static_cast<int>(d.refv[i]);
            std::snprintf(row[i + 1], sizeof(row[i + 1]), "%d %u/%u %+d", i, d.sig[i], d.refv[i],
                          delta);
        }
    } else if (!d.iqs_ok) {
        std::snprintf(row[0], sizeof(row[0]), "Pad");
        std::snprintf(row[1], sizeof(row[1]), "no trackpad");
    } else {
        uint8_t bitmap[ocfw::kScreenBytes];
        ocfw::drawCounts(d.ch, d.ch_ok, d.fingers, bitmap);
        Show(bitmap);
        return;
    }
    const char* lines[4] = {row[0], row[1], row[2], row[3]};
    uint8_t bitmap[ocfw::kScreenBytes];
    ocfw::drawLines(lines, bitmap);
    Show(bitmap);
}

void setup() {
    session.reset();
    ocfw::InitSwitches();
    ocfw::Input boot;
    ocfw::ReadSwitches(boot);
    feed.seed(boot);

    // CDC must be added before the host reads the descriptor. Opening that
    // port at 1200 baud and dropping DTR reboots into the ROM bootloader,
    // which is how `pio run -t upload` writes without the BOOT button.
    Serial.begin(115200);

    // USB before the display. A missing OLED only NACKs; a stuck bus must
    // not be what keeps the board from enumerating.
    if (!TinyUSBDevice.isInitialized()) TinyUSBDevice.begin(0);
    TinyUSBDevice.setManufacturerDescriptor("OpenChord");
    TinyUSBDevice.setProductDescriptor("OpenChord M1");
    usb_midi.setStringDescriptor("OpenChord M1");
    MIDI.setHandleNoteOn(OnNoteOn);
    MIDI.setHandleNoteOff(OnNoteOff);
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.turnThruOff();
    ocfw::InitTrs(OnNoteOn, OnNoteOff);
    // Hold the trackpad in reset until the display has claimed the bus.
    // Its NRST pull-up would otherwise let it clock-stretch the OLED.
    pinMode(ocboard::kIqsNrst, OUTPUT);
    digitalWrite(ocboard::kIqsNrst, LOW);
    ocfw::InitOled();
    ocfw::InitTouch();
}

void loop() {
#ifdef TINYUSB_NEED_POLLING_TASK
    TinyUSBDevice.task();
#endif

    ocfw::Input in;
    ocfw::ReadSwitches(in);
    ocfw::ReadTouch(in);
    if (debug_page == 1) ocfw::SampleStripChannels();

    // Keys, Scale, Drums, then Strip / Strip ch / Pad. The engine only has
    // the three play modes, so these pages stay in the firmware.
    static ocfw::Input seen{};
    static bool have_seen = false;
    bool swallow = false;
    if (have_seen && !session.menuOpen()) {
        const bool next = in.next && !seen.next;
        const bool prev = in.prev && !seen.prev;
        const bool menu = in.menu && !seen.menu;
        if (debug_page >= 0 && menu) {
            debug_page = -1;
        } else if (next) {
            if (debug_page >= 0) {
                if (++debug_page >= kDebugPages) {
                    debug_page = -1;
                    session.setPlayMode(oc::PlayMode::Keys);
                }
                swallow = true;
            } else if (session.playMode() == oc::PlayMode::Drums) {
                debug_page = 0;
                swallow = true;
            }
        } else if (prev) {
            if (debug_page >= 0) {
                if (--debug_page < 0) session.setPlayMode(oc::PlayMode::Drums);
                swallow = true;
            } else if (session.playMode() == oc::PlayMode::Keys) {
                debug_page = kDebugPages - 1;
                swallow = true;
            }
        }
    }
    seen = in;
    have_seen = true;

    if (debug_page >= 0 || swallow) feed.seed(in);
    if (debug_page < 0) {
        feed.apply(session, in);
        FlushMidi();
    }

    if (TinyUSBDevice.mounted()) MIDI.read();
    ocfw::ReadTrs();

    if (debug_page < 0) Paint();
    else PaintDebug();
}
