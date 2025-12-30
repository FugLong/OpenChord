#pragma once

/**
 * Global configuration flags for OpenChord firmware
 * 
 * Set these flags to control build-time behavior across all modules
 */

// Debug mode flag - when true:
//   - Disables USB MIDI (to allow serial logging over USB)
//   - Enables serial connection delay in main
//   - Enables verbose debug output
// 
// Set to true for debugging, false for normal operation
#define DEBUG_MODE false

// Debug screen flag - when true:
//   - Enables debug screen system on the OLED display
//   - Shows various debug views (system status, inputs, analog, audio, MIDI)
//   - Allows navigation between views using encoder
// 
// Set to true to enable debug screen, false to disable
// When disabled, the display can be used by production UI
#define DEBUG_SCREEN_ENABLED true

