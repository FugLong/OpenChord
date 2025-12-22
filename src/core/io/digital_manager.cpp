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
    // Key matrix layout: 3 rows x 4 columns (Bottom: 4 keys, Middle: 3 keys, Top: 4 keys)
    // Physical layout: Row 0 = bottom, Row 1 = middle, Row 2 = top
    // 
    // NOTE: We use daisy::seed::DXX constants instead of hw->GetPin() because
    // GetPin() expects physical pin numbers (1-40), but for pins 27-33, the physical
    // pin numbers don't match the Daisy pin names. Physical Pin 27 = D20, not D27.
    // Using GetPin(27) would fail to initialize the GPIO correctly.
    //
    // Physical Pin 27 = D20, Pin 28 = D21, Pin 29 = D22
    // Physical Pin 30 = D23, Pin 31 = D24, Pin 32 = D25, Pin 33 = D26
    key_matrix_row_pins_[0] = daisy::seed::D20; // Pin 27 - Row 0 (Bottom row)
    key_matrix_row_pins_[1] = daisy::seed::D21; // Pin 28 - Row 1 (Middle row)
    key_matrix_row_pins_[2] = daisy::seed::D22; // Pin 29 - Row 2 (Top row)
    
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
    // NOTE: Joystick button moved to D0 (Pin 1) - D14/D15 now used for display DC/RST
    joystick_button_pin_ = daisy::seed::D0;  // Pin 1 - Joystick button (was D14, moved for display)
    // audio_switch_pin_ = daisy::seed::D15;    // Pin 15 - Audio switch (disabled, pin used for display RST)
    
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
    // audio_switch_gpio_.Init(audio_switch_pin_, daisy::GPIO::Mode::INPUT, daisy::GPIO::Pull::PULLUP); // Disabled - pin used for display
    
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
// Simple microsecond delay using busy-wait loop
// Daisy Seed runs at 480MHz, so we need approximately 480 cycles per microsecond
// This is a simple approximation - adjust multiplier if needed for accuracy
static inline void delay_us(uint32_t us) {
    // Each iteration is roughly 3-4 cycles, so ~120 iterations per microsecond at 480MHz
    // Using volatile to prevent compiler optimization
    volatile uint32_t count = us * 120;
    while (count--) {
        __asm__("nop"); // No operation instruction
    }
}

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
    
    // Small delay to ensure all rows are fully deactivated before scanning
    // This prevents cross-talk between rows
    delay_us(10); // 10 microseconds for signal stabilization
    
    // Scan each row
    // Matrix layout (physical):
    // - Row 0 (bottom): 4 columns (0-3) - all valid, (0,0)=bottom-left, (0,3)=bottom-right
    // - Row 1 (middle): 3 columns (0-2) - column 3 is NOT connected to this row, (1,0)=middle-left, (1,2)=middle-right
    // - Row 2 (top): 4 columns (0-3) - all valid, (2,0)=top-left, (2,3)=top-right
    // Column 3 is only connected to switches from R0 and R2, not R1
    for (int row = 0; row < KEY_MATRIX_ROWS; row++) {
        // Activate current row (set LOW)
        key_matrix_rows_[row].Write(false); // LOW = active
        
        // Critical: Wait for signal to stabilize after activating row
        // This ensures the column inputs have time to settle to correct state
        // Without this delay, we may read columns before they've fully responded to the row activation
        // This is especially important to prevent cross-talk between rows
        delay_us(5); // 5 microseconds for row activation to stabilize
        
        // Read all columns for this row
        // Only process valid key positions to avoid false readings
        for (int col = 0; col < KEY_MATRIX_COLS; col++) {
            // Only read and update state for valid key positions
            // This prevents reading invalid positions (like R1C3) which could cause issues
            if (IsValidKeyPosition(row, col)) {
                // Column has pullup, so:
                // - HIGH = no key pressed (no connection to LOW row)
                // - LOW = key pressed (LOW row connects through diode to column)
                // Diodes prevent ghosting: current can only flow from column to row
                bool raw_read = key_matrix_cols_[col].Read();
                bool current_pressed = !raw_read; // Inverted due to pullup
                
                // Update button state with debouncing
                UpdateButtonState(key_matrix_.keys[row][col], current_pressed);
            }
        }
        
        // Deactivate row (set HIGH) before moving to next row
        key_matrix_rows_[row].Write(true); // HIGH = inactive
        
        // Small delay after deactivating to ensure clean transition
        // This prevents the next row scan from picking up residual signals from this row
        // This is critical to prevent cross-talk, especially between R1 and R2
        delay_us(5); // 5 microseconds for row deactivation to complete
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
    
    // Update joystick button (now on D0/Pin 1)
    bool joystick_current = !joystick_button_gpio_.Read(); // Inverted due to pullup
    UpdateButtonState(joystick_button_, joystick_current);
    
    // Audio switch disabled - pin now used for display RST
    // bool audio_current = !audio_switch_gpio_.Read(); // Inverted due to pullup
    // UpdateButtonState(audio_switch_, audio_current);
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
    // Physical layout: Bottom row (Row 0, 4 keys), Middle row (Row 1, 3 keys), Top row (Row 2, 4 keys)
    // The unused position is Row 1, Col 3 (middle row, rightmost position - not physically connected)
    return !(row == 1 && col == 3);
}

void DigitalManager::UpdateButtonState(ButtonState& button, bool current_pressed) {
    // Time-based debouncing: require debounce_time_ms_ to pass before state change
    // This prevents false triggers from electrical noise or contact bounce
    
    uint32_t now = hw_->system.GetNow();
    bool previous_pressed = button.pressed;
    
    // Clear was_pressed at the start of each update cycle
    // This ensures it's only true for ONE scan cycle after a press is detected
    button.was_pressed = false;
    
    // If state changed, check if enough time has passed since last change
    if (current_pressed != button.pressed) {
        uint32_t time_since_change = now - (current_pressed ? button.last_release_time : button.last_press_time);
        if (time_since_change < debounce_time_ms_) {
            // Not enough time - keep previous state (debouncing)
            return;
        }
    }
    
    // Detect rising edge: was_pressed is true only for ONE scan cycle when transitioning from false to true
    // Only set was_pressed if we're transitioning from not pressed to pressed
    if (!previous_pressed && current_pressed) {
        button.was_pressed = true;
    }
    
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
