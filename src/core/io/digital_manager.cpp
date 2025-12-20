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
    
    // Initialize pin assignments - using Daisy pin constants directly
    // Key matrix layout: 3 rows x 4 columns (Top: 4 keys, Middle: 3 keys, Bottom: 4 keys)
    // 
    // NOTE: We use daisy::seed::DXX constants instead of hw->GetPin() because
    // GetPin() expects physical pin numbers (1-40), but for pins 27-33, the physical
    // pin numbers don't match the Daisy pin names. Physical Pin 27 = D20, not D27.
    // Using GetPin(27) would fail to initialize the GPIO correctly.
    //
    // Physical Pin 27 = D20, Pin 28 = D21, Pin 29 = D22
    // Physical Pin 30 = D23, Pin 31 = D24, Pin 32 = D25, Pin 33 = D26
    key_matrix_row_pins_[0] = daisy::seed::D20; // Pin 27 - Row 0 (Top row)
    key_matrix_row_pins_[1] = daisy::seed::D21; // Pin 28 - Row 1 (Middle row)
    key_matrix_row_pins_[2] = daisy::seed::D22; // Pin 29 - Row 2 (Bottom row)
    
    // Key matrix columns
    key_matrix_col_pins_[0] = daisy::seed::D23; // Pin 30 - Col 0
    key_matrix_col_pins_[1] = daisy::seed::D24; // Pin 31 - Col 1
    key_matrix_col_pins_[2] = daisy::seed::D25; // Pin 32 - Col 2
    key_matrix_col_pins_[3] = daisy::seed::D26; // Pin 33 - Col 3
    
    // Encoder pins (pins 34-35)
    // Using Daisy pin constants directly (same as matrix pins)
    encoder_a_pin_ = daisy::seed::D27; // Pin 34 - Encoder A
    encoder_b_pin_ = daisy::seed::D28; // Pin 35 - Encoder B
    // Note: encoder_button_pin_ not connected in current design
    
    // Other digital inputs
    joystick_button_pin_ = daisy::seed::D14;  // Pin 14 - Joystick button
    audio_switch_pin_ = daisy::seed::D15;    // Pin 15 - Audio switch
    
    // Initialize GPIO objects
    for (int i = 0; i < KEY_MATRIX_ROWS; i++) {
        key_matrix_rows_[i].Init(key_matrix_row_pins_[i], daisy::GPIO::Mode::OUTPUT, daisy::GPIO::Pull::NOPULL);
        key_matrix_rows_[i].Write(true); // Start with all rows HIGH (inactive)
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
    
    // Keyboard matrix scanning with diodes:
    // - Rows are outputs (driven LOW when active)
    // - Columns are inputs with pullups (read HIGH when no key, LOW when key pressed)
    // - Diodes prevent ghosting (allow current flow from column to row)
    //   Diode orientation: Anode → Column, Cathode → Row
    // - Scan one row at a time, read all columns
    
    // Set all rows HIGH (inactive) first - ensures clean state before scanning
    for (int row = 0; row < KEY_MATRIX_ROWS; row++) {
        key_matrix_rows_[row].Write(true); // HIGH = inactive
    }
    
    // Scan each row
    for (int row = 0; row < KEY_MATRIX_ROWS; row++) {
        // Activate current row (set LOW)
        key_matrix_rows_[row].Write(false); // LOW = active
        
        // Read all columns for this row
        for (int col = 0; col < KEY_MATRIX_COLS; col++) {
            if (IsValidKeyPosition(row, col)) {
                // Column has pullup, so:
                // - HIGH = no key pressed (no connection to LOW row)
                // - LOW = key pressed (LOW row connects through diode to column)
                bool raw_read = key_matrix_cols_[col].Read();
                bool current_pressed = !raw_read; // Inverted due to pullup
                
                // Update button state with debouncing
                UpdateButtonState(key_matrix_.keys[row][col], current_pressed);
            }
        }
        
        // Deactivate row (set HIGH) before moving to next row
        key_matrix_rows_[row].Write(true); // HIGH = inactive
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
    
    // Check if this position is used (11 keys in 3x4 matrix)
    // Layout: Top row (4 keys), Middle row (3 keys), Bottom row (4 keys)
    // The unused position is Row 1, Col 3 (middle row, rightmost position)
    return !(row == 1 && col == 3);
}

void DigitalManager::UpdateButtonState(ButtonState& button, bool current_pressed) {
    // Time-based debouncing: require debounce_time_ms_ to pass before state change
    // This prevents false triggers from electrical noise or contact bounce
    
    uint32_t now = hw_->system.GetNow();
    bool previous_pressed = button.pressed;
    
    // If state changed, check if enough time has passed since last change
    if (current_pressed != button.pressed) {
        uint32_t time_since_change = now - (current_pressed ? button.last_release_time : button.last_press_time);
        if (time_since_change < debounce_time_ms_) {
            // Not enough time - keep previous state (debouncing)
            return;
        }
    }
    
    // Detect rising edge: was_pressed is true only for ONE scan cycle when transitioning from false to true
    button.was_pressed = (!previous_pressed && current_pressed);
    
    // Update pressed state
    button.pressed = current_pressed;
    
    // Update timing and hold state
    if (button.pressed) {
        if (button.hold_time == 0) {
            button.last_press_time = now;
        }
        button.hold_time++;
    } else {
        button.last_release_time = now;
        button.hold_time = 0;
    }
}
