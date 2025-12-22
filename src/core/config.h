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
#define DEBUG_MODE true

