#pragma once

#include "daisy_seed.h"
#include <cstdint>

class IO {
public:
    static constexpr int NUM_BUTTONS = 8;  // Adjust based on actual hardware
    
    IO();
    ~IO();
    
    // Initialization
    void Init(daisy::DaisySeed* hw);
    void Update();
    
    // Encoder handling
    float GetEncoderDelta() const;
    int GetEncoderValue() const;
    void SetEncoderValue(int value);
    
    // Button handling
    bool IsButtonPressed(int button) const;
    bool WasButtonPressed(int button) const;
    bool IsButtonHeld(int button) const;
    uint32_t GetButtonHoldTime(int button) const;
    
    // Joystick handling
    void GetJoystick(float* x, float* y) const;
    float GetJoystickX() const;
    float GetJoystickY() const;
    
    // LED control (if available)
    void SetLED(int led, bool state);
    void SetLEDBrightness(int led, float brightness);
    
private:
    daisy::DaisySeed* hw_;
    
    // Encoder
    daisy::Encoder encoder_;
    
    // Button pins
    daisy::GPIO button_pins_[NUM_BUTTONS];
    
    // Encoder state
    int encoder_value_;
    int last_encoder_value_;
    float encoder_delta_;
    
    // Button state
    bool button_states_[NUM_BUTTONS];
    bool last_button_states_[NUM_BUTTONS];
    uint32_t button_hold_times_[NUM_BUTTONS];
    
    // Joystick state
    float joystick_x_;
    float joystick_y_;
    
    // LED state
    bool led_states_[4];  // Assuming 4 LEDs
    float led_brightness_[4];
    
    // Internal methods
    void UpdateEncoder();
    void UpdateButtons();
    void UpdateJoystick();
    void UpdateLEDs();
}; 