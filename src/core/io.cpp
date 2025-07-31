#include "../include/io.h"
#include <cmath>

IO::IO() : hw_(nullptr), encoder_value_(0), last_encoder_value_(0), encoder_delta_(0.0f),
           joystick_x_(0.0f), joystick_y_(0.0f) {
    // Initialize button states
    for (int i = 0; i < NUM_BUTTONS; i++) {
        button_states_[i] = false;
        last_button_states_[i] = false;
        button_hold_times_[i] = 0;
    }
    
    // Initialize LED states
    for (int i = 0; i < 4; i++) {
        led_states_[i] = false;
        led_brightness_[i] = 0.0f;
    }
}

IO::~IO() {
}

void IO::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    
    // Initialize encoder on pins 34 and 35 (D27 and D28 according to pinout)
    // Note: Using a dummy pin for click since it's not connected
    encoder_.Init(daisy::seed::D27, daisy::seed::D28, daisy::seed::D26);
    
    // Initialize buttons (using available GPIO pins)
    // Based on pinout: buttons on pins 14, 15, 27-33 (key matrix)
    // For now, using a subset of available pins
    button_pins_[0].Init(daisy::seed::D13, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
    button_pins_[1].Init(daisy::seed::D14, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
    button_pins_[2].Init(daisy::seed::D15, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
    button_pins_[3].Init(daisy::seed::D26, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
    button_pins_[4].Init(daisy::seed::D29, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
    button_pins_[5].Init(daisy::seed::D30, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
    button_pins_[6].Init(daisy::seed::D31, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
    button_pins_[7].Init(daisy::seed::D32, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
    
    // Initialize ADC for joystick (pins 24 and 25 according to pinout)
    daisy::AdcChannelConfig adc_cfg[2];
    adc_cfg[0].InitSingle(daisy::seed::A2);  // Joystick X (pin 24)
    adc_cfg[1].InitSingle(daisy::seed::A3);  // Joystick Y (pin 25)
    
    hw_->adc.Init(adc_cfg, 2);
    hw_->adc.Start();
}

void IO::Update() {
    UpdateEncoder();
    UpdateButtons();
    UpdateJoystick();
    UpdateLEDs();
}

void IO::UpdateEncoder() {
    if (!hw_) return;
    
    encoder_.Debounce();
    int increment = encoder_.Increment();
    
    last_encoder_value_ = encoder_value_;
    encoder_value_ += increment;
    encoder_delta_ = static_cast<float>(increment);
}

void IO::UpdateButtons() {
    if (!hw_) return;
    
    for (int i = 0; i < NUM_BUTTONS; i++) {
        last_button_states_[i] = button_states_[i];
        button_states_[i] = !button_pins_[i].Read();  // Inverted for pull-up
        
        if (button_states_[i]) {
            button_hold_times_[i]++;
        } else {
            button_hold_times_[i] = 0;
        }
    }
}

void IO::UpdateJoystick() {
    if (!hw_) return;
    
    // Read analog inputs for joystick
    // Convert ADC values (0-65535) to -1 to 1 range
    joystick_x_ = (hw_->adc.GetFloat(0) - 0.5f) * 2.0f;  // Convert to -1 to 1
    joystick_y_ = (hw_->adc.GetFloat(1) - 0.5f) * 2.0f;  // Convert to -1 to 1
    
    // Apply deadzone
    const float deadzone = 0.1f;
    if (std::abs(joystick_x_) < deadzone) joystick_x_ = 0.0f;
    if (std::abs(joystick_y_) < deadzone) joystick_y_ = 0.0f;
}

void IO::UpdateLEDs() {
    if (!hw_) return;
    
    // Update LED states if LEDs are available
    // This would depend on the specific hardware configuration
}

float IO::GetEncoderDelta() const {
    return encoder_delta_;
}

int IO::GetEncoderValue() const {
    return encoder_value_;
}

void IO::SetEncoderValue(int value) {
    encoder_value_ = value;
    last_encoder_value_ = value;
    encoder_delta_ = 0.0f;
}

bool IO::IsButtonPressed(int button) const {
    if (button < 0 || button >= NUM_BUTTONS) return false;
    return button_states_[button];
}

bool IO::WasButtonPressed(int button) const {
    if (button < 0 || button >= NUM_BUTTONS) return false;
    return last_button_states_[button];
}

bool IO::IsButtonHeld(int button) const {
    if (button < 0 || button >= NUM_BUTTONS) return false;
    return button_hold_times_[button] > 1000;  // 1 second hold time
}

uint32_t IO::GetButtonHoldTime(int button) const {
    if (button < 0 || button >= NUM_BUTTONS) return 0;
    return button_hold_times_[button];
}

void IO::GetJoystick(float* x, float* y) const {
    if (x) *x = joystick_x_;
    if (y) *y = joystick_y_;
}

float IO::GetJoystickX() const {
    return joystick_x_;
}

float IO::GetJoystickY() const {
    return joystick_y_;
}

void IO::SetLED(int led, bool state) {
    if (led < 0 || led >= 4) return;
    led_states_[led] = state;
}

void IO::SetLEDBrightness(int led, float brightness) {
    if (led < 0 || led >= 4) return;
    led_brightness_[led] = std::max(0.0f, std::min(1.0f, brightness));
} 