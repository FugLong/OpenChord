#include "digital_manager.h"
#include <cstring>

DigitalManager::DigitalManager() 
    : hw_(nullptr), debounce_time_ms_(10), hold_threshold_ms_(500), healthy_(true) {
    
    // Initialize key matrix state
    memset(&key_matrix_, 0, sizeof(key_matrix_));
    key_matrix_.matrix_healthy = true;
    
    // Initialize encoder state
    memset(&encoder_state_, 0, sizeof(encoder_state_));
    
    // Initialize button states
    memset(&joystick_button_, 0, sizeof(joystick_button_));
    memset(&audio_switch_, 0, sizeof(audio_switch_));
    
    // Initialize LED states
    memset(led_states_, 0, sizeof(led_states_));
    memset(led_brightness_, 0, sizeof(led_brightness_));
}

DigitalManager::~DigitalManager() {
    Shutdown();
}

void DigitalManager::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    
    // Initialize pin assignments based on pinout.md
    // Key matrix rows (pins 27-30)
    key_matrix_row_pins_[0] = hw->GetPin(27); // Row 0
    key_matrix_row_pins_[1] = hw->GetPin(28); // Row 1
    key_matrix_row_pins_[2] = hw->GetPin(29); // Row 2
    key_matrix_row_pins_[3] = hw->GetPin(30); // Row 3
    
    // Key matrix columns (pins 31-33)
    key_matrix_col_pins_[0] = hw->GetPin(31); // Col 0
    key_matrix_col_pins_[1] = hw->GetPin(32); // Col 1
    key_matrix_col_pins_[2] = hw->GetPin(33); // Col 2
    
    // Encoder pins (pins 34-35)
    encoder_a_pin_ = hw->GetPin(34);
    encoder_b_pin_ = hw->GetPin(35);
    // Note: encoder_button_pin_ not connected in current design
    
    // Other digital inputs
    joystick_button_pin_ = hw->GetPin(14);
    audio_switch_pin_ = hw->GetPin(15);
    
    // Initialize GPIO objects
    for (int i = 0; i < KEY_MATRIX_ROWS; i++) {
        key_matrix_rows_[i].Init(key_matrix_row_pins_[i], daisy::GPIO::Mode::OUTPUT);
        key_matrix_rows_[i].Write(false); // Start with all rows low
    }
    
    for (int i = 0; i < KEY_MATRIX_COLS; i++) {
        key_matrix_cols_[i].Init(key_matrix_col_pins_[i], daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
    }
    
    // Initialize encoder (a, b, click, update_rate)
    // Note: encoder_button_pin_ is not connected in current design, so we pass a dummy pin
    encoder_.Init(encoder_a_pin_, encoder_b_pin_, encoder_a_pin_, 0.0f);
    
    // Initialize other buttons
    joystick_button_gpio_.Init(joystick_button_pin_, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
    audio_switch_gpio_.Init(audio_switch_pin_, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP);
    
    // Initialize LEDs (if implemented)
    for (int i = 0; i < 4; i++) {
        // TODO: Initialize LED pins when implemented
        // led_gpios_[i].Init(led_pins_[i], daisy::GPIO::Mode::OUTPUT);
    }
    
    healthy_ = true;
}

void DigitalManager::Update() {
    if (!hw_) return;
    
    UpdateKeyMatrix();
    UpdateEncoder();
    UpdateButtons();
    UpdateLEDs();
}

void DigitalManager::Shutdown() {
    if (!hw_) return;
    
    // Shutdown all GPIO operations
    healthy_ = false;
    hw_ = nullptr;
}

// Key Matrix methods
bool DigitalManager::IsKeyPressed(int row, int col) const {
    if (!IsValidKeyPosition(row, col)) return false;
    return key_matrix_.keys[row][col].pressed;
}

bool DigitalManager::WasKeyPressed(int row, int col) const {
    if (!IsValidKeyPosition(row, col)) return false;
    return key_matrix_.keys[row][col].was_pressed;
}

bool DigitalManager::IsKeyHeld(int row, int col) const {
    if (!IsValidKeyPosition(row, col)) return false;
    return key_matrix_.keys[row][col].pressed && 
           key_matrix_.keys[row][col].hold_time > hold_threshold_ms_;
}

uint32_t DigitalManager::GetKeyHoldTime(int row, int col) const {
    if (!IsValidKeyPosition(row, col)) return 0;
    return key_matrix_.keys[row][col].hold_time;
}

const KeyMatrixState& DigitalManager::GetKeyMatrix() const {
    return key_matrix_;
}

// LED methods
void DigitalManager::SetLED(int led, bool state) {
    if (led < 0 || led >= 4) return;
    led_states_[led] = state;
    // TODO: Update actual LED when implemented
}

void DigitalManager::SetLEDBrightness(int led, float brightness) {
    if (led < 0 || led >= 4) return;
    led_brightness_[led] = brightness;
    // TODO: Update actual LED when implemented
}

bool DigitalManager::GetLED(int led) const {
    if (led < 0 || led >= 4) return false;
    return led_states_[led];
}

float DigitalManager::GetLEDBrightness(int led) const {
    if (led < 0 || led >= 4) return 0.0f;
    return led_brightness_[led];
}

// Utility methods
void DigitalManager::ResetEncoder(int value) {
    encoder_state_.value = value;
    encoder_state_.last_value = value;
    encoder_state_.delta = 0.0f;
}

void DigitalManager::SetKeyMatrixDebounceTime(uint32_t ms) {
    debounce_time_ms_ = ms;
}

void DigitalManager::SetButtonHoldThreshold(uint32_t ms) {
    hold_threshold_ms_ = ms;
}

// Private methods
void DigitalManager::UpdateKeyMatrix() {
    if (!hw_) return;
    
    // Scan the key matrix
    ScanKeyMatrix();
    
    // Update button states and timing
    for (int row = 0; row < KEY_MATRIX_ROWS; row++) {
        for (int col = 0; col < KEY_MATRIX_COLS; col++) {
            if (IsValidKeyPosition(row, col)) {
                // Get current state from GPIO
                bool current_pressed = !key_matrix_cols_[col].Read(); // Inverted due to pullup
                UpdateButtonState(key_matrix_.keys[row][col], current_pressed);
            }
        }
    }
    
    key_matrix_.scan_count++;
    key_matrix_.last_scan_time = hw_->system.GetNow();
}

void DigitalManager::UpdateEncoder() {
    if (!hw_) return;
    
    encoder_.Debounce();
    int current_value = encoder_.Increment();
    
    if (current_value != encoder_state_.last_value) {
        encoder_state_.delta = static_cast<float>(current_value - encoder_state_.last_value);
        encoder_state_.value = current_value;
        encoder_state_.last_value = current_value;
    } else {
        encoder_state_.delta = 0.0f;
    }
    
    // TODO: Handle encoder button when implemented
    // encoder_state_.button_pressed = encoder_button_gpio_.Read();
}

void DigitalManager::UpdateButtons() {
    if (!hw_) return;
    
    // Update joystick button
    bool joystick_current = !joystick_button_gpio_.Read(); // Inverted due to pullup
    UpdateButtonState(joystick_button_, joystick_current);
    
    // Update audio switch
    bool audio_current = !audio_switch_gpio_.Read(); // Inverted due to pullup
    UpdateButtonState(audio_switch_, audio_current);
}

void DigitalManager::UpdateLEDs() {
    if (!hw_) return;
    
    // TODO: Update actual LED states when implemented
    // For now, just keep the internal state
}

void DigitalManager::ScanKeyMatrix() {
    // Matrix scanning is handled in UpdateKeyMatrix()
    // This method is kept for future expansion
}

bool DigitalManager::IsValidKeyPosition(int row, int col) const {
    // Check bounds
    if (row < 0 || row >= KEY_MATRIX_ROWS || col < 0 || col >= KEY_MATRIX_COLS) {
        return false;
    }
    
    // Check if this position is used (11 keys in 4x3 matrix)
    // The unused position is typically (3,2) - adjust as needed
    if (row == 3 && col == 2) {
        return false; // Unused position
    }
    
    return true;
}

void DigitalManager::UpdateButtonState(ButtonState& button, bool current_pressed) {
    button.was_pressed = button.pressed;
    button.pressed = current_pressed;
    
    if (button.pressed) {
        if (button.hold_time == 0) {
            button.last_press_time = hw_->system.GetNow();
        }
        button.hold_time++;
    } else {
        button.last_release_time = hw_->system.GetNow();
        button.hold_time = 0;
    }
}
