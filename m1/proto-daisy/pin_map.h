#pragma once

// Pins for the original OpenChord Seed box as it is actually wired.
//
// Source of truth: archive/s1-daisy Init() in
//   src/core/io/digital_manager.cpp
//   src/core/io/analog_manager.cpp
//   src/core/io/display_manager.cpp
//   src/core/midi/midi_handler.cpp
//
// Do NOT copy archive/s1-daisy/src/core/io/pin_config.h
// Do NOT copy archive/s1-daisy/docs/hardware/pinout.md
// Both are stale. pin_config.h still has joystick on D14, display DC on D0,
// a 4-row/3-col matrix, and MIDI D12/D13 names that do not match midi_handler.cpp.
//
// Daisy names vs header pins: physical pin 27 is D20, not D27.

#include "daisy_seed.h"

namespace m1proto {

struct PinMap {
    // Analog (analog_manager.cpp)
    static constexpr daisy::Pin VOLUME     = daisy::seed::A0;  // pin 22
    static constexpr daisy::Pin MIC        = daisy::seed::A1;  // pin 23
    static constexpr daisy::Pin JOY_X      = daisy::seed::A2;  // pin 24 left/right
    static constexpr daisy::Pin JOY_Y      = daisy::seed::A3;  // pin 25 up/down
    static constexpr daisy::Pin BATT       = daisy::seed::A4;  // pin 26

    // Key matrix 3x4 (digital_manager.cpp)
    // Row 0 bottom 4 keys, row 1 middle 3 keys (col 3 unused), row 2 top 4 keys
    static constexpr daisy::Pin KEY_ROW_0  = daisy::seed::D20; // pin 27 bottom
    static constexpr daisy::Pin KEY_ROW_1  = daisy::seed::D21; // pin 28 middle
    static constexpr daisy::Pin KEY_ROW_2  = daisy::seed::D22; // pin 29 top
    static constexpr daisy::Pin KEY_COL_0  = daisy::seed::D23; // pin 30
    static constexpr daisy::Pin KEY_COL_1  = daisy::seed::D24; // pin 31
    static constexpr daisy::Pin KEY_COL_2  = daisy::seed::D25; // pin 32
    static constexpr daisy::Pin KEY_COL_3  = daisy::seed::D26; // pin 33

    // Joystick click moved off D14 so display can use D13/D14
    static constexpr daisy::Pin JOY_SW     = daisy::seed::D0;  // pin 1

    // Encoder (no click)
    static constexpr daisy::Pin ENC_A      = daisy::seed::D27; // pin 34
    static constexpr daisy::Pin ENC_B      = daisy::seed::D28; // pin 35

    // OLED SSD1306 4-wire SPI (display_manager.cpp)
    static constexpr daisy::Pin OLED_CS    = daisy::seed::D7;  // pin 8
    static constexpr daisy::Pin OLED_SCK   = daisy::seed::D8;  // pin 9
    static constexpr daisy::Pin OLED_MOSI  = daisy::seed::D10; // pin 11
    static constexpr daisy::Pin OLED_DC    = daisy::seed::D13; // pin 14
    static constexpr daisy::Pin OLED_RST   = daisy::seed::D14; // pin 15

    // TRS MIDI UART4 (midi_handler.cpp uses PORTB 8/9, not seed::D12/D13)
    static constexpr daisy::Pin MIDI_RX    = daisy::Pin(daisy::PORTB, 8); // pin 12 = D11
    static constexpr daisy::Pin MIDI_TX    = daisy::Pin(daisy::PORTB, 9); // pin 13 = D12

    // USB MIDI: EXTERNAL = D29/D30 = pins 36-37
};

} // namespace m1proto
