#pragma once

#include <cstdint>

namespace OpenChord {

/**
 * MIDI event structure for internal use
 */
struct MidiEvent {
    enum Type {
        NOTE_OFF = 0x80,
        NOTE_ON = 0x90,
        POLY_AFTERTOUCH = 0xA0,
        CONTROL_CHANGE = 0xB0,
        PROGRAM_CHANGE = 0xC0,
        CHANNEL_AFTERTOUCH = 0xD0,
        PITCH_BEND = 0xE0,
        SYSEX = 0xF0,
        CLOCK = 0xF8,
        START = 0xFA,
        CONTINUE = 0xFB,
        STOP = 0xFC,
        ACTIVE_SENSING = 0xFE,
        RESET = 0xFF
    };

    uint8_t type;
    uint8_t channel;
    uint8_t data1;  // Note, controller number, etc.
    uint8_t data2;  // Velocity, controller value, etc.
    uint32_t timestamp;  // Sample-accurate timestamp
};

/**
 * MIDI message for external MIDI I/O
 */
struct MidiMessage {
    uint8_t data[3];
    uint8_t length;
    uint32_t timestamp;
};

} // namespace OpenChord 