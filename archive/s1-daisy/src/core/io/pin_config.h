#pragma once

#include "daisy_seed.h"

namespace OpenChord {

/**
 * Centralized pin configuration for all hardware
 * 
 * This file defines all pin assignments in one place, making it easy to:
 * - See what's connected where
 * - Modify pin assignments without hunting through code
 * - Ensure consistency across all managers
 * - Document hardware connections
 */
struct PinConfig {
    // ADC Pins (Analog Inputs) - Based on pinout.md
    static constexpr daisy::Pin VOLUME_POT = daisy::seed::A0;    // Pin 22 - Volume potentiometer
    static constexpr daisy::Pin MICROPHONE = daisy::seed::A1;    // Pin 23 - Microphone input
    static constexpr daisy::Pin JOYSTICK_X = daisy::seed::A2;    // Pin 24 - Joystick X axis
    static constexpr daisy::Pin JOYSTICK_Y = daisy::seed::A3;    // Pin 25 - Joystick Y axis
    static constexpr daisy::Pin BATTERY_MON = daisy::seed::A4;   // Pin 26 - Battery voltage monitor
    
    // Key Matrix Pins - Based on pinout.md
    // Note: Physical pin numbers don't match Daisy pin names!
    // Pin 27 = D20, Pin 28 = D21, Pin 29 = D22, Pin 30 = D23
    // Pin 31 = D24, Pin 32 = D25, Pin 33 = D26
    static constexpr daisy::Pin KEY_ROW_0 = daisy::seed::D20;    // Pin 27 - Matrix row 0
    static constexpr daisy::Pin KEY_ROW_1 = daisy::seed::D21;    // Pin 28 - Matrix row 1
    static constexpr daisy::Pin KEY_ROW_2 = daisy::seed::D22;    // Pin 29 - Matrix row 2
    static constexpr daisy::Pin KEY_ROW_3 = daisy::seed::D23;    // Pin 30 - Matrix row 3
    static constexpr daisy::Pin KEY_COL_0 = daisy::seed::D24;    // Pin 31 - Matrix column 0
    static constexpr daisy::Pin KEY_COL_1 = daisy::seed::D25;    // Pin 32 - Matrix column 1
    static constexpr daisy::Pin KEY_COL_2 = daisy::seed::D26;    // Pin 33 - Matrix column 2
    
    // Joystick Button - Based on pinout.md
    static constexpr daisy::Pin JOYSTICK_SW = daisy::seed::D14;  // Pin 14 - Joystick button
    
    // Encoder Pins - Based on pinout.md
    static constexpr daisy::Pin ENCODER_A = daisy::seed::D27;    // Pin 34 - Encoder A
    static constexpr daisy::Pin ENCODER_B = daisy::seed::D28;    // Pin 35 - Encoder B
    
    // Audio Switch - Based on pinout.md
    static constexpr daisy::Pin AUDIO_SWITCH = daisy::seed::D15; // Pin 15 - TRS input select
    
    // Display Pins - Based on pinout.md (SPI1 interface)
    // Note: Physical pin numbers don't match Daisy pin names!
    // Pin 8 = D7 (SPI1_NSS), Pin 9 = D8 (SPI1_SCK), Pin 11 = D10 (SPI1_MOSI)
    static constexpr daisy::Pin DISPLAY_CS = daisy::seed::D7;    // Pin 8 - SPI1_NSS (chip select, GPIO controlled)
    static constexpr daisy::Pin DISPLAY_DC = daisy::seed::D0;    // Pin 1 - D0 (data/command GPIO)
    static constexpr daisy::Pin DISPLAY_SCK = daisy::seed::D8;   // Pin 9 - SPI1_SCK (clock)
    static constexpr daisy::Pin DISPLAY_MOSI = daisy::seed::D10; // Pin 11 - SPI1_MOSI (data out)
    
    // SD Card Pins - Based on pinout.md
    static constexpr daisy::Pin SD_DAT3 = daisy::seed::D2;       // Pin 2 - SD card DAT3
    static constexpr daisy::Pin SD_DAT2 = daisy::seed::D3;       // Pin 3 - SD card DAT2
    static constexpr daisy::Pin SD_DAT1 = daisy::seed::D4;       // Pin 4 - SD card DAT1
    static constexpr daisy::Pin SD_DAT0 = daisy::seed::D5;       // Pin 5 - SD card DAT0
    static constexpr daisy::Pin SD_CMD = daisy::seed::D6;        // Pin 6 - SD card CMD
    static constexpr daisy::Pin SD_CK = daisy::seed::D7;         // Pin 7 - SD card clock
    
    // MIDI UART Pins - Based on pinout.md
    static constexpr daisy::Pin MIDI_RX = daisy::seed::D12;      // Pin 12 - MIDI input (UART4_RX)
    static constexpr daisy::Pin MIDI_TX = daisy::seed::D13;      // Pin 13 - MIDI output (UART4_TX)
    
    // Audio Pins (handled by Daisy automatically)
    // - Pin 16: AUDIO_IN_L
    // - Pin 17: AUDIO_IN_R  
    // - Pin 18: AUDIO_OUT_L
    // - Pin 19: AUDIO_OUT_R
    // - Pin 20: AGND
    // - Pin 21: +3V3A (power for joystick)
    
    // USB Pins (handled by Daisy automatically)
    // - Pin 36: USB_HS_D-
    // - Pin 37: USB_HS_D+
    
    // System Pins (handled by Daisy automatically)
    // - Pin 38: +3V3D (power for display/SD)
    // - Pin 39: VIN
    // - Pin 40: GND
};

} // namespace OpenChord
