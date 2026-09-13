#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include "hardware/structs/sio.h"
#include "pico/time.h"
#include "session.h"

#ifndef PIN_NEOPIXEL
#define PIN_NEOPIXEL 16
#endif

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

static oc::Session session;
static volatile uint32_t midi_flash_ms = 0;
static void MidiSaw() { midi_flash_ms = millis() + 120; }

static void NeoSend(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t bits = (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(r) << 8) | b;
    const uint32_t mask = 1u << PIN_NEOPIXEL;
    noInterrupts();
    for (int i = 23; i >= 0; --i) {
        sio_hw->gpio_set = mask;
        if (bits & (1u << i)) {
            busy_wait_at_least_cycles(90);
            sio_hw->gpio_clr = mask;
            busy_wait_at_least_cycles(40);
        } else {
            busy_wait_at_least_cycles(32);
            sio_hw->gpio_clr = mask;
            busy_wait_at_least_cycles(90);
        }
    }
    interrupts();
    busy_wait_us(80);
}

static void FlushMidi() {
    oc::MidiEvent ev[32];
    int n = session.drain(ev, 32);
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
    }
}

static bool AnyChord() {
    // Approximate: white LED when type held or session has a chord name.
    char name[20];
    session.chordName(name, sizeof(name));
    return name[0] != 0;
}

static void LedTask() {
    uint32_t now = millis();
    if (static_cast<int32_t>(midi_flash_ms - now) > 0) {
        NeoSend(0, 80, 0);
        return;
    }
    if (session.heldType() != oc::Type::None || AnyChord()) {
        NeoSend(80, 80, 80);
        return;
    }
    if (TinyUSBDevice.mounted()) {
        NeoSend(0, 12, 24);
        return;
    }
    NeoSend((now / 400) & 1 ? 20 : 0, 0, 0);
}

static void ApplyPadCc(byte number, bool on) {
    switch (number) {
        case 36: session.typePad(0, on); break;
        case 37: session.typePad(1, on); break;
        case 38: session.typePad(2, on); break;
        case 39: session.typePad(3, on); break;
        case 40: session.setExtBit(oc::Ext6, on); break;
        case 41: session.setExtBit(oc::Extm7, on); break;
        case 42: session.setExtBit(oc::ExtM7, on); break;
        case 43: session.setExtBit(oc::Ext9, on); break;
        default: break;
    }
    FlushMidi();
}

static void OnNoteOn(byte channel, byte pitch, byte velocity) {
    if (channel == 10) return;
    MidiSaw();
    session.noteOn(channel, pitch, velocity);
    FlushMidi();
}

static void OnNoteOff(byte channel, byte pitch, byte velocity) {
    (void)velocity;
    if (channel == 10) return;
    session.noteOff(channel, pitch);
    FlushMidi();
}

static void OnCc(byte channel, byte number, byte value) {
    (void)channel;
    if (number >= 36 && number <= 43) {
        ApplyPadCc(number, value >= 64);
        return;
    }
    MidiSaw();
    if (number == 47) session.stickCcX(value);
    else if (number == 48) session.stickCcY(value);
    session.updateSeat();
    FlushMidi();
}

void setup() {
    session.reset();

    pinMode(PIN_NEOPIXEL, OUTPUT);
    digitalWrite(PIN_NEOPIXEL, LOW);
    NeoSend(40, 0, 40);

    if (!TinyUSBDevice.isInitialized()) {
        TinyUSBDevice.begin(0);
    }
    TinyUSBDevice.setManufacturerDescriptor("OpenChord");
    TinyUSBDevice.setProductDescriptor("OpenChord M1");
    usb_midi.setStringDescriptor("OpenChord M1");
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.setHandleNoteOn(OnNoteOn);
    MIDI.setHandleNoteOff(OnNoteOff);
    MIDI.setHandleControlChange(OnCc);
}

void loop() {
#ifdef TINYUSB_NEED_POLLING_TASK
    TinyUSBDevice.task();
#endif
    LedTask();
    if (!TinyUSBDevice.mounted()) return;

    MIDI.read();
    session.updateSeat();
    FlushMidi();
}
