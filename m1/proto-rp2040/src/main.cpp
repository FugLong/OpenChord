#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include "hardware/structs/sio.h"
#include "pico/time.h"
#include "chord_engine.h"

#ifndef PIN_NEOPIXEL
#define PIN_NEOPIXEL 16
#endif

Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

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

// Each piano key keeps the assignment it got at note-on until that key is released.
struct Voice {
    int16_t root; // -1 = empty
    bool chord;
    oc::Type type;
    uint8_t ext;
    oc::Voicing v;
};

static constexpr int kMaxVoices = 16;

static Voice voices[kMaxVoices];
static uint8_t note_refs[128];
static uint8_t held_vel = 100;
static uint8_t out_ch = 1;
static uint8_t key_pc = 0;

static uint8_t type_stack[4];
static uint8_t type_stack_n = 0;
static uint8_t pad_ext = 0;
static float stick_x = 0.f;
static float stick_y = 0.f;
static bool stick_x_armed = false;
static bool stick_y_armed = false;
static oc::Seat last_seat = oc::Seat::Home;

static oc::Type HeldType() {
    if (type_stack_n == 0) return oc::Type::None;
    return static_cast<oc::Type>(type_stack[type_stack_n - 1]);
}

static void TypePad(uint8_t idx, bool on) {
    uint8_t t = static_cast<uint8_t>(idx + 1); // Dim=1 ... Sus=4
    if (on) {
        for (uint8_t i = 0; i < type_stack_n; ++i) {
            if (type_stack[i] == t) {
                for (uint8_t j = i; j + 1 < type_stack_n; ++j) type_stack[j] = type_stack[j + 1];
                type_stack_n--;
                break;
            }
        }
        if (type_stack_n < 4) type_stack[type_stack_n++] = t;
        return;
    }
    uint8_t w = 0;
    for (uint8_t i = 0; i < type_stack_n; ++i) {
        if (type_stack[i] != t) type_stack[w++] = type_stack[i];
    }
    type_stack_n = w;
}

static void MidiSendOn(uint8_t note, uint8_t vel) {
    if (note > 127) return;
    if (vel == 0) vel = 1;
    if (note_refs[note] < 255) note_refs[note]++;
    if (note_refs[note] == 1) MIDI.sendNoteOn(note, vel, out_ch);
}

static void MidiSendOff(uint8_t note) {
    if (note > 127) return;
    if (!note_refs[note]) return;
    note_refs[note]--;
    if (note_refs[note] == 0) MIDI.sendNoteOff(note, 0, out_ch);
}

static void VoicingOff(const oc::Voicing& v) {
    for (uint8_t i = 0; i < v.n; ++i) MidiSendOff(v.notes[i]);
}

static void VoicingOn(const oc::Voicing& v, uint8_t vel) {
    for (uint8_t i = 0; i < v.n; ++i) MidiSendOn(v.notes[i], vel);
}

static bool VoicingHas(const oc::Voicing& v, uint8_t note) {
    for (uint8_t i = 0; i < v.n; ++i) {
        if (v.notes[i] == note) return true;
    }
    return false;
}

static void DiffVoicing(oc::Voicing& cur, const oc::Voicing& next, uint8_t vel) {
    bool same = (cur.n == next.n);
    if (same) {
        for (uint8_t i = 0; i < next.n; ++i) {
            if (cur.notes[i] != next.notes[i]) same = false;
        }
    }
    if (same) {
        cur = next;
        return;
    }
    for (uint8_t i = 0; i < cur.n; ++i) {
        if (!VoicingHas(next, cur.notes[i])) MidiSendOff(cur.notes[i]);
    }
    for (uint8_t i = 0; i < next.n; ++i) {
        if (!VoicingHas(cur, next.notes[i])) MidiSendOn(next.notes[i], vel);
    }
    cur = next;
}

static void RenderVoice(Voice& h) {
    if (!h.chord || h.root < 0) return;
    oc::EngineInput in{};
    in.root_midi = h.root;
    in.type = h.type;
    in.ext = h.ext;
    in.seat = oc::SeatFromStick(stick_x, stick_y);
    in.key_pc = key_pc;
    in.velocity = held_vel;
    oc::Voicing next{};
    oc::Render(in, h.v.n ? &h.v : nullptr, &next);
    if (h.v.n == 0) {
        h.v = next;
        VoicingOn(h.v, held_vel);
        return;
    }
    DiffVoicing(h.v, next, held_vel);
}

static Voice* FindVoice(int16_t root) {
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices[i].root == root) return &voices[i];
    }
    return nullptr;
}

static Voice* AllocVoice() {
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices[i].root < 0) return &voices[i];
    }
    return nullptr;
}

static bool AnyChord() {
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices[i].root >= 0 && voices[i].chord) return true;
    }
    return false;
}

static bool TypeStillHeld(oc::Type t) {
    uint8_t want = static_cast<uint8_t>(t);
    for (uint8_t i = 0; i < type_stack_n; ++i) {
        if (type_stack[i] == want) return true;
    }
    return false;
}

static void RefreshHeldChordExts() {
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices[i].root < 0 || !voices[i].chord) continue;
        if (!TypeStillHeld(voices[i].type)) continue;
        voices[i].ext = pad_ext;
        RenderVoice(voices[i]);
    }
}

static void LedTask() {
    uint32_t now = millis();
    if (static_cast<int32_t>(midi_flash_ms - now) > 0) {
        NeoSend(0, 80, 0);
        return;
    }
    if (HeldType() != oc::Type::None || AnyChord()) {
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
        case 36:
            TypePad(0, on);
            break;
        case 37:
            TypePad(1, on);
            break;
        case 38:
            TypePad(2, on);
            break;
        case 39:
            TypePad(3, on);
            break;
        case 40:
            if (on) pad_ext |= oc::Ext6;
            else pad_ext = static_cast<uint8_t>(pad_ext & ~oc::Ext6);
            break;
        case 41:
            if (on) pad_ext |= oc::Extm7;
            else pad_ext = static_cast<uint8_t>(pad_ext & ~oc::Extm7);
            break;
        case 42:
            if (on) pad_ext |= oc::ExtM7;
            else pad_ext = static_cast<uint8_t>(pad_ext & ~oc::ExtM7);
            break;
        case 43:
            if (on) pad_ext |= oc::Ext9;
            else pad_ext = static_cast<uint8_t>(pad_ext & ~oc::Ext9);
            break;
        default:
            break;
    }
    if (number >= 40 && number <= 43) RefreshHeldChordExts();
}

static void HandleNote(byte channel, byte pitch, byte velocity, bool on) {
    if (channel == 10) return;

    MidiSaw();
    out_ch = channel;
    int16_t root = static_cast<int16_t>(pitch);

    if (on) {
        if (FindVoice(root)) return;
        Voice* h = AllocVoice();
        if (!h) return;
        held_vel = velocity ? velocity : held_vel;
        oc::Type t = HeldType();
        h->root = root;
        h->v.n = 0;
        if (t != oc::Type::None) {
            h->chord = true;
            h->type = t;
            h->ext = pad_ext;
            RenderVoice(*h);
        } else {
            h->chord = false;
            h->type = oc::Type::None;
            h->ext = 0;
            MidiSendOn(pitch, velocity);
        }
        return;
    }

    Voice* h = FindVoice(root);
    if (!h) return;
    if (h->chord) VoicingOff(h->v);
    else MidiSendOff(pitch);
    h->root = -1;
    h->v.n = 0;
}

static void OnNoteOn(byte channel, byte pitch, byte velocity) {
    if (velocity == 0) HandleNote(channel, pitch, velocity, false);
    else HandleNote(channel, pitch, velocity, true);
}

static void OnNoteOff(byte channel, byte pitch, byte velocity) {
    HandleNote(channel, pitch, velocity, false);
}

static float CcToStick(byte value) { return (static_cast<float>(value) - 64.0f) / 64.0f; }

// Knobs rest at 0. Ignore them until they pass near center once, then track.
static void StickCc(bool* armed, float* axis, byte value) {
    if (!*armed) {
        if (value < 48 || value > 80) return;
        *armed = true;
    }
    *axis = CcToStick(value);
}

static void OnCc(byte channel, byte number, byte value) {
    (void)channel;
    if (number >= 36 && number <= 43) {
        ApplyPadCc(number, value >= 64);
        return;
    }
    MidiSaw();
    if (number == 47) StickCc(&stick_x_armed, &stick_x, value);
    else if (number == 48) StickCc(&stick_y_armed, &stick_y, value);
}

void setup() {
    for (int i = 0; i < kMaxVoices; ++i) voices[i].root = -1;

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

    oc::Seat seat = oc::SeatFromStick(stick_x, stick_y);
    if (seat == last_seat) return;
    last_seat = seat;
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices[i].root >= 0 && voices[i].chord) RenderVoice(voices[i]);
    }
}
