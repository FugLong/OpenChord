#pragma once

#include "digital_manager.h"
#include <cstdint>

namespace OpenChord {

/**
 * Button Input Handler - High-level button input system
 * 
 * Provides semantic button access and mode support for the 11-button matrix.
 * Separates musical buttons (7 keys) from system buttons (4 keys) and provides
 * a clean interface for different input modes and future extensibility.
 * 
 * Button Layout:
 * - Musical Buttons (7): Row 0 (4 white keys) + Row 1 (3 black keys)
 *   - Used for MIDI notes, drum pads, and other musical input
 * - System Buttons (4): Row 2 (top row)
 *   - Play/Pause, Input selection, Instrument, FX, Record
 */

// Semantic button IDs for musical buttons (7 keys)
enum class MusicalButton {
    // Bottom row (white keys) - Row 0
    WHITE_0 = 0,  // (0,0) - Leftmost white key
    WHITE_1 = 1,  // (0,1)
    WHITE_2 = 2,  // (0,2)
    WHITE_3 = 3,  // (0,3) - Rightmost white key
    
    // Middle row (black keys) - Row 1
    BLACK_0 = 4,  // (1,0) - Leftmost black key
    BLACK_1 = 5,  // (1,1)
    BLACK_2 = 6,  // (1,2) - Rightmost black key
    
    COUNT = 7
};

// Semantic button IDs for system buttons (4 keys)
enum class SystemButton {
    INPUT = 0,         // (2,0) - Input selection (also play/pause loop)
    INSTRUMENT = 1,    // (2,1) - Instrument selection/options
    FX = 2,            // (2,2) - FX selection/options
    RECORD = 3,        // (2,3) - Record start/stop, loop settings
    
    COUNT = 4
};

// Input modes for musical buttons
enum class InputMode {
    MIDI_NOTES,        // Standard MIDI note generation
    DRUM_PADS,         // Drum pad mode (samples)
    // Future modes:
    // CHORD_MODE,     // Chord generation
    // ARP_MODE,       // Arpeggiator
    // SEQUENCER,      // Step sequencer
};

// Button event types
enum class ButtonEventType {
    PRESSED,           // Button was just pressed
    RELEASED,          // Button was just released
    HELD,              // Button is being held (after hold threshold)
};

// Button event structure
struct ButtonEvent {
    ButtonEventType type;
    uint32_t timestamp;
    union {
        MusicalButton musical_button;
        SystemButton system_button;
    };
    bool is_musical;  // true if musical button, false if system button
};

/**
 * Button Input Handler
 * 
 * Provides high-level access to button inputs with mode support and
 * semantic button naming. Designed for extensibility and future features.
 */
class ButtonInputHandler {
public:
    ButtonInputHandler();
    ~ButtonInputHandler();
    
    // Initialization
    void Init(::DigitalManager* digital_manager);
    void Update();
    
    // Mode management
    void SetInputMode(InputMode mode) { input_mode_ = mode; }
    InputMode GetInputMode() const { return input_mode_; }
    
    // Musical button access (semantic)
    bool IsMusicalButtonPressed(MusicalButton button) const;
    bool WasMusicalButtonPressed(MusicalButton button) const;
    bool IsMusicalButtonHeld(MusicalButton button) const;
    uint32_t GetMusicalButtonHoldTime(MusicalButton button) const;
    
    // System button access (semantic)
    bool IsSystemButtonPressed(SystemButton button) const;
    bool WasSystemButtonPressed(SystemButton button) const;
    bool IsSystemButtonHeld(SystemButton button) const;
    uint32_t GetSystemButtonHoldTime(SystemButton button) const;
    
    // Low-level access (for advanced use cases)
    bool IsRawButtonPressed(int row, int col) const;
    bool WasRawButtonPressed(int row, int col) const;
    
    // Event polling (for future callback/event system)
    // Returns true if an event occurred, fills in the event structure
    bool PollEvent(ButtonEvent& event);
    
    // Check if there are pending events (for power management)
    bool HasPendingEvents() const { return event_queue_count_ > 0; }
    
    // Configuration
    void SetHoldThreshold(uint32_t ms);
    uint32_t GetHoldThreshold() const { return hold_threshold_ms_; }

private:
    ::DigitalManager* digital_manager_;
    InputMode input_mode_;
    uint32_t hold_threshold_ms_;
    
    // Event queue (simple for now, can be expanded later)
    static constexpr int MAX_EVENTS = 16;
    ButtonEvent event_queue_[MAX_EVENTS];
    int event_queue_head_;
    int event_queue_tail_;
    int event_queue_count_;
    
    // Helper methods
    void ProcessMusicalButtons();
    void ProcessSystemButtons();
    void QueueEvent(const ButtonEvent& event);
    
    // Convert semantic button IDs to row/col
    void MusicalButtonToRowCol(MusicalButton button, int& row, int& col) const;
    void SystemButtonToRowCol(SystemButton button, int& row, int& col) const;
    
    // Track previous states for edge detection
    bool prev_musical_states_[static_cast<int>(MusicalButton::COUNT)];
    bool prev_system_states_[static_cast<int>(SystemButton::COUNT)];
};

} // namespace OpenChord

