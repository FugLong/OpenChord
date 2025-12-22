#pragma once

#include "analog_manager.h"
#include <cstdint>

namespace OpenChord {

/**
 * Joystick Input Handler - High-level joystick input system
 * 
 * Provides semantic joystick access with mode support and event handling.
 * The joystick is used for navigation, chord mapping, and other expressive controls.
 * 
 * Joystick Layout:
 * - X-axis: Left/Right movement (typically for chord selection, navigation)
 * - Y-axis: Up/Down movement (typically for scale selection, parameter control)
 */

// Joystick input modes
enum class JoystickMode {
    NAVIGATION,        // Standard navigation (UI, menus)
    CHORD_MAPPING,     // Chord generation and mapping
    PARAMETER_CONTROL, // Parameter adjustment
    // Future modes:
    // SCALE_MAPPING,  // Scale selection
    // ARP_CONTROL,    // Arpeggiator control
};

// Joystick event types
enum class JoystickEventType {
    MOVED,            // Joystick moved (position changed)
    CENTERED,         // Joystick returned to center
    EDGE_REACHED,     // Reached edge of range
    // Future events:
    // GESTURE,        // Detected gesture pattern
};

// Joystick event structure
struct JoystickEvent {
    JoystickEventType type;
    float x;              // Current X position (-1.0 to 1.0, centered at 0.0)
    float y;              // Current Y position (-1.0 to 1.0, centered at 0.0)
    float delta_x;        // Change in X since last event
    float delta_y;        // Change in Y since last event
    uint32_t timestamp;
};

// Joystick direction (for discrete navigation)
enum class JoystickDirection {
    CENTER,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    UP_LEFT,
    UP_RIGHT,
    DOWN_LEFT,
    DOWN_RIGHT
};

/**
 * Joystick Input Handler
 * 
 * Provides high-level access to joystick inputs with mode support,
 * event handling, and discrete direction detection.
 */
class JoystickInputHandler {
public:
    JoystickInputHandler();
    ~JoystickInputHandler();
    
    // Initialization
    void Init(::AnalogManager* analog_manager);
    void Update();
    
    // Mode management
    void SetMode(JoystickMode mode) { mode_ = mode; }
    JoystickMode GetMode() const { return mode_; }
    
    // Continuous position access
    float GetX() const;              // X position (-1.0 to 1.0, centered at 0.0)
    float GetY() const;              // Y position (-1.0 to 1.0, centered at 0.0)
    void GetPosition(float* x, float* y) const;
    
    // Delta (change) access
    float GetDeltaX() const;         // Change in X since last update
    float GetDeltaY() const;         // Change in Y since last update
    
    // Discrete direction detection
    JoystickDirection GetDirection() const;
    bool IsCentered() const;
    bool IsAtEdge() const;
    
    // Threshold-based direction (for navigation)
    // Returns true if joystick is pushed in a specific direction beyond threshold
    bool IsPushedUp(float threshold = 0.3f) const;
    bool IsPushedDown(float threshold = 0.3f) const;
    bool IsPushedLeft(float threshold = 0.3f) const;
    bool IsPushedRight(float threshold = 0.3f) const;
    
    // Event polling
    bool PollEvent(JoystickEvent& event);
    
    // Configuration
    void SetDeadZone(float dead_zone);      // Dead zone threshold (0.0 to 0.5)
    void SetMovementThreshold(float threshold); // Minimum movement to trigger events
    float GetDeadZone() const { return dead_zone_; }
    float GetMovementThreshold() const { return movement_threshold_; }

private:
    ::AnalogManager* analog_manager_;
    JoystickMode mode_;
    float dead_zone_;
    float movement_threshold_;
    
    // Current state
    float current_x_;
    float current_y_;
    float prev_x_;
    float prev_y_;
    
    // Event queue
    static constexpr int MAX_EVENTS = 8;
    JoystickEvent event_queue_[MAX_EVENTS];
    int event_queue_head_;
    int event_queue_tail_;
    int event_queue_count_;
    
    // Helper methods
    void ProcessJoystick();
    void QueueEvent(const JoystickEvent& event);
    JoystickDirection CalculateDirection(float x, float y) const;
    bool HasMoved(float threshold) const;
};

} // namespace OpenChord

