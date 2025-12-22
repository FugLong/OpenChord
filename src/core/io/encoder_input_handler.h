#pragma once

#include "digital_manager.h"
#include <cstdint>

namespace OpenChord {

/**
 * Encoder Input Handler - High-level rotary encoder input system
 * 
 * Provides semantic encoder access with mode support and event handling.
 * The encoder is used for menu navigation, parameter adjustment, and value selection.
 * 
 * Encoder Features:
 * - Rotation: Increment/decrement with acceleration support
 * - Button: Press detection (when implemented)
 */

// Encoder input modes
enum class EncoderMode {
    NAVIGATION,        // Menu/item navigation
    PARAMETER_ADJUST,  // Parameter value adjustment
    VALUE_SELECT,      // Value selection (e.g., note selection)
    // Future modes:
    // SCROLL,          // Scrolling through lists
    // ZOOM,            // Zoom in/out
};

// Encoder event types
enum class EncoderEventType {
    ROTATED,           // Encoder was rotated (increment or decrement)
    BUTTON_PRESSED,    // Encoder button was pressed
    BUTTON_RELEASED,   // Encoder button was released
    BUTTON_HELD,       // Encoder button is being held
};

// Encoder event structure
struct EncoderEvent {
    EncoderEventType type;
    int delta;              // Rotation delta (positive = clockwise, negative = counter-clockwise)
    int value;              // Current encoder value
    uint32_t timestamp;
};

// Encoder rotation direction
enum class EncoderDirection {
    NONE,
    CLOCKWISE,         // Increment
    COUNTER_CLOCKWISE  // Decrement
};

/**
 * Encoder Input Handler
 * 
 * Provides high-level access to encoder inputs with mode support,
 * event handling, and acceleration support.
 */
class EncoderInputHandler {
public:
    EncoderInputHandler();
    ~EncoderInputHandler();
    
    // Initialization
    void Init(::DigitalManager* digital_manager);
    void Update();
    
    // Mode management
    void SetMode(EncoderMode mode) { mode_ = mode; }
    EncoderMode GetMode() const { return mode_; }
    
    // Value access
    int GetValue() const;              // Current encoder value
    float GetDelta() const;           // Rotation delta (with acceleration)
    int GetRawDelta() const;          // Raw rotation delta (no acceleration)
    
    // Direction detection
    EncoderDirection GetDirection() const;
    bool IsRotating() const;          // True if encoder is currently rotating
    
    // Button access (when implemented)
    bool IsButtonPressed() const;
    bool WasButtonPressed() const;
    bool IsButtonHeld() const;
    uint32_t GetButtonHoldTime() const;
    
    // Event polling
    bool PollEvent(EncoderEvent& event);
    
    // Configuration
    void Reset(int value = 0);       // Reset encoder value
    void SetAccelerationEnabled(bool enabled);
    bool IsAccelerationEnabled() const { return acceleration_enabled_; }
    void SetAccelerationThreshold(int threshold); // Steps before acceleration kicks in

private:
    ::DigitalManager* digital_manager_;
    EncoderMode mode_;
    bool acceleration_enabled_;
    int acceleration_threshold_;
    
    // Current state
    int current_value_;
    int prev_value_;
    float current_delta_;
    EncoderDirection current_direction_;
    
    // Event queue
    static constexpr int MAX_EVENTS = 16;
    EncoderEvent event_queue_[MAX_EVENTS];
    int event_queue_head_;
    int event_queue_tail_;
    int event_queue_count_;
    
    // Acceleration tracking
    int rotation_steps_;              // Consecutive rotation steps
    uint32_t last_rotation_time_;    // Time of last rotation
    
    // Helper methods
    void ProcessEncoder();
    void ProcessButton();
    void QueueEvent(const EncoderEvent& event);
    float CalculateAcceleration(int steps) const;
};

} // namespace OpenChord

