#include "trs.h"

#include <Arduino.h>
#include <MIDI.h>

#include "board.h"

// Serial1 is the RP2040 UART0 block. GPIO0 and GPIO1 are a legal UART0 pair.
// 31250 8N1 comes from the MIDI library's serial transport. No invert: the
// Type A output idles with TX high, and the TLP2361 already presents a low
// for a MIDI 0.

namespace ocfw {
namespace {

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, trs);

TrsNoteFn note_on_ = nullptr;
TrsNoteFn note_off_ = nullptr;

void OnNoteOn(byte channel, byte pitch, byte velocity) {
    if (note_on_) note_on_(channel, pitch, velocity);
}

void OnNoteOff(byte channel, byte pitch, byte velocity) {
    if (note_off_) note_off_(channel, pitch, velocity);
}

} // namespace

void InitTrs(TrsNoteFn on, TrsNoteFn off) {
    note_on_ = on;
    note_off_ = off;
    Serial1.setPinout(ocboard::kMidiTx, ocboard::kMidiRx);
    Serial1.setInvertTX(false);
    Serial1.setInvertRX(false);
    trs.setHandleNoteOn(OnNoteOn);
    trs.setHandleNoteOff(OnNoteOff);
    trs.begin(MIDI_CHANNEL_OMNI);
    trs.turnThruOff();
}

void ReadTrs() { trs.read(); }

void SendTrs(const oc::MidiEvent& ev) {
    switch (ev.kind) {
        case oc::MidiEvent::Kind::NoteOn:
            trs.sendNoteOn(ev.data1, ev.data2, ev.channel);
            break;
        case oc::MidiEvent::Kind::NoteOff:
            trs.sendNoteOff(ev.data1, ev.data2, ev.channel);
            break;
        case oc::MidiEvent::Kind::Cc:
            trs.sendControlChange(ev.data1, ev.data2, ev.channel);
            break;
    }
}

} // namespace ocfw
