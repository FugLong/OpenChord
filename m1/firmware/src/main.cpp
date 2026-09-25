#include <Arduino.h>

#include <cstring>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

#include "input.h"
#include "oled.h"
#include "screen.h"
#include "session.h"
#include "switches.h"
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

static void Paint() {
    ocfw::ScreenText text;
    session.fillScreen(text.top, sizeof(text.top), text.left, sizeof(text.left), text.mid,
                       sizeof(text.mid), text.right, sizeof(text.right), &text.zones, &text.zone);
    static ocfw::ScreenText shown{};
    static bool have = false;
    if (have && std::memcmp(&shown, &text, sizeof(text)) == 0 && ocfw::OledReady()) return;
    uint8_t bitmap[ocfw::kScreenBytes];
    ocfw::drawScreen(text, bitmap);
    if (ocfw::PresentOled(bitmap)) {
        shown = text;
        have = true;
    } else if (!ocfw::OledReady()) {
        ocfw::InitOled();
    }
}

void setup() {
    session.reset();
    ocfw::InitSwitches();
    ocfw::InitOled();

    if (!TinyUSBDevice.isInitialized()) TinyUSBDevice.begin(0);
    TinyUSBDevice.setManufacturerDescriptor("OpenChord");
    TinyUSBDevice.setProductDescriptor("OpenChord M1");
    usb_midi.setStringDescriptor("OpenChord M1");
    MIDI.setHandleNoteOn(OnNoteOn);
    MIDI.setHandleNoteOff(OnNoteOff);
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.turnThruOff();
    ocfw::InitTrs(OnNoteOn, OnNoteOff);
}

void loop() {
#ifdef TINYUSB_NEED_POLLING_TASK
    TinyUSBDevice.task();
#endif

    ocfw::Input in;
    ocfw::ReadSwitches(in);
    // Trackpad and strip stay at rest. They share I2C with the OLED.
    feed.apply(session, in);
    FlushMidi();

    if (TinyUSBDevice.mounted()) MIDI.read();
    ocfw::ReadTrs();

    Paint();
}
