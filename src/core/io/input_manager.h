#pragma once

#include "button_input_handler.h"
#include "joystick_input_handler.h"
#include "encoder_input_handler.h"
#include "io_manager.h"

namespace OpenChord {

/**
 * Input Manager - Unified high-level input system
 * 
 * Provides a single interface for accessing all input devices:
 * - Button matrix (11 buttons: 7 musical + 4 system)
 * - Joystick (X/Y axes)
 * - Rotary encoder
 * 
 * This class coordinates all input handlers and provides a clean,
 * unified API for the rest of the system. It enables:
 * - Coordinated input handling (e.g., modifier keys + other inputs)
 * - Mode management across all inputs
 * - Event aggregation from all input sources
 * - Simplified access for plugins and UI systems
 */
class InputManager {
public:
    InputManager();
    ~InputManager();
    
    // Initialization
    void Init(IOManager* io_manager);
    void Update();
    
    // Button access
    ButtonInputHandler& GetButtons() { return button_handler_; }
    const ButtonInputHandler& GetButtons() const { return button_handler_; }
    
    // Joystick access
    JoystickInputHandler& GetJoystick() { return joystick_handler_; }
    const JoystickInputHandler& GetJoystick() const { return joystick_handler_; }
    
    // Encoder access
    EncoderInputHandler& GetEncoder() { return encoder_handler_; }
    const EncoderInputHandler& GetEncoder() const { return encoder_handler_; }
    
    // Convenience methods for common operations
    
    // Check if any system button is pressed (for modifier keys)
    bool IsAnySystemButtonPressed() const;
    
    // Check if a specific system button is pressed
    bool IsSystemButtonPressed(SystemButton button) const {
        return button_handler_.IsSystemButtonPressed(button);
    }
    
    // Get current input mode for buttons
    InputMode GetButtonInputMode() const {
        return button_handler_.GetInputMode();
    }
    
    // Set input mode for buttons
    void SetButtonInputMode(InputMode mode) {
        button_handler_.SetInputMode(mode);
    }
    
    // Get joystick mode
    JoystickMode GetJoystickMode() const {
        return joystick_handler_.GetMode();
    }
    
    // Set joystick mode
    void SetJoystickMode(JoystickMode mode) {
        joystick_handler_.SetMode(mode);
    }
    
    // Get encoder mode
    EncoderMode GetEncoderMode() const {
        return encoder_handler_.GetMode();
    }
    
    // Set encoder mode
    void SetEncoderMode(EncoderMode mode) {
        encoder_handler_.SetMode(mode);
    }
    
    // Unified mode setting (sets mode for all inputs)
    void SetAllModes(InputMode button_mode, JoystickMode joystick_mode, EncoderMode encoder_mode) {
        SetButtonInputMode(button_mode);
        SetJoystickMode(joystick_mode);
        SetEncoderMode(encoder_mode);
    }
    
    // Health check
    bool IsHealthy() const;

private:
    IOManager* io_manager_;
    
    // Individual input handlers
    ButtonInputHandler button_handler_;
    JoystickInputHandler joystick_handler_;
    EncoderInputHandler encoder_handler_;
    
    // Initialization state
    bool initialized_;
};

} // namespace OpenChord

