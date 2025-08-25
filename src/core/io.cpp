#include "../../include/io.h"
#include <cstring>

IO::IO() 
    : hw_(nullptr), encoder_value_(0), last_encoder_value_(0), encoder_delta_(0.0f),
      joystick_x_(0.0f), joystick_y_(0.0f) {
    
    // Initialize button states
    memset(button_states_, 0, sizeof(button_states_));
    memset(last_button_states_, 0, sizeof(last_button_states_));
    memset(button_hold_times_, 0, sizeof(button_hold_times_));
    
    // Initialize LED states
    memset(led_states_, 0, sizeof(led_states_));
    memset(led_brightness_, 0, sizeof(led_brightness_));
    
    // Initialize ADC configuration
    memset(adc_configured_, 0, sizeof(adc_configured_));
}

IO::~IO() {
}

void IO::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    
    // Initialize encoder
    encoder_.Init(hw->GetPin(15), hw->GetPin(16), hw->GetPin(17)); // Example pins
    
    // Initialize button pins (example pins - adjust based on actual hardware)
    for (int i = 0; i < NUM_BUTTONS; i++) {
        button_pins_[i].Init(hw->GetPin(18 + i), daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
    }
    
    // Initialize LED pins (example pins - adjust based on actual hardware)
    for (int i = 0; i < 4; i++) {
        // LED pins would be initialized here if available
    }
}

void IO::Update() {
    UpdateEncoder();
    UpdateButtons();
    UpdateJoystick();
    UpdateLEDs();
}

// ADC handling methods
float IO::GetADCValue(int channel) const {
    if (!hw_ || channel < 0 || channel >= NUM_ADC_CHANNELS || !adc_configured_[channel]) {
        return 0.0f;
    }
    
    // Get ADC value from Daisy hardware
    return hw_->adc.GetFloat(channel);
}

void IO::ConfigureADC(int channel, daisy::Pin pin) {
    if (channel < 0 || channel >= NUM_ADC_CHANNELS) {
        return;
    }
    
    // Configure ADC channel
    adc_configs_[channel].InitSingle(pin);
    adc_configured_[channel] = true;
    
    // If this is the first ADC channel, initialize the ADC system
    if (channel == 0) {
        hw_->adc.Init(adc_configs_, NUM_ADC_CHANNELS);
        hw_->adc.Start();
    }
}

// Encoder methods
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

// Button methods
bool IO::IsButtonPressed(int button) const {
    if (button < 0 || button >= NUM_BUTTONS) return false;
    return button_states_[button];
}

bool IO::WasButtonPressed(int button) const {
    if (button < 0 || button >= NUM_BUTTONS) return false;
    return button_states_[button] && !last_button_states_[button];
}

bool IO::IsButtonHeld(int button) const {
    if (button < 0 || button >= NUM_BUTTONS) return false;
    return button_states_[button] && button_hold_times_[button] > 500; // 500ms hold time
}

uint32_t IO::GetButtonHoldTime(int button) const {
    if (button < 0 || button >= NUM_BUTTONS) return 0;
    return button_hold_times_[button];
}

// Joystick methods
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

// LED methods
void IO::SetLED(int led, bool state) {
    if (led < 0 || led >= 4) return;
    led_states_[led] = state;
}

void IO::SetLEDBrightness(int led, float brightness) {
    if (led < 0 || led >= 4) return;
    led_brightness_[led] = brightness;
}

// Private update methods
void IO::UpdateEncoder() {
    if (!hw_) return;
    
    encoder_.Debounce();
    int current_value = encoder_.Increment();
    
    if (current_value != last_encoder_value_) {
        encoder_delta_ = static_cast<float>(current_value - last_encoder_value_);
        encoder_value_ = current_value;
        last_encoder_value_ = current_value;
    } else {
        encoder_delta_ = 0.0f;
    }
}

void IO::UpdateButtons() {
    if (!hw_) return;
    
    for (int i = 0; i < NUM_BUTTONS; i++) {
        last_button_states_[i] = button_states_[i];
        button_states_[i] = !button_pins_[i].Read(); // Inverted due to pullup
        
        // Update hold times
        if (button_states_[i]) {
            button_hold_times_[i] += 1; // Increment hold time
        } else {
            button_hold_times_[i] = 0; // Reset hold time
        }
    }
}

void IO::UpdateJoystick() {
    if (!hw_) return;
    
    // TODO: Implement actual joystick reading
    // For now, just keep current values
    // joystick_x_ = hw_->adc.GetFloat(joystick_x_channel);
    // joystick_y_ = hw_->adc.GetFloat(joystick_y_channel);
}

void IO::UpdateLEDs() {
    if (!hw_) return;
    
    // TODO: Implement actual LED control
    // For now, just keep current states
} 