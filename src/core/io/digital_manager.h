#pragma once

#include "daisy_seed.h"
#include <cstdint>

// Key matrix dimensions
// Physical layout: Bottom row (Row 0, 4 keys), Middle row (Row 1, 3 keys), Top row (Row 2, 4 keys)
static constexpr int KEY_MATRIX_ROWS = 3;
static constexpr int KEY_MATRIX_COLS = 4;
static constexpr int KEY_MATRIX_KEYS = 11; // 3x4 matrix with one unused position (Row 1, Col 3)

// Button states and timing
struct ButtonState {
    bool pressed;
    bool was_pressed;
    uint32_t hold_time;
    uint32_t last_press_time;
    uint32_t last_release_time;
};

// Key matrix state
struct KeyMatrixState {
    ButtonState keys[KEY_MATRIX_ROWS][KEY_MATRIX_COLS];
    bool matrix_healthy;
    uint32_t scan_count;
    uint32_t last_scan_time;
};

// Encoder state
struct EncoderState {
    int value;
    int last_value;
    float delta;
    bool button_pressed;
    bool button_was_pressed;
    uint32_t button_hold_time;
};

/**
 * Digital Manager - Handles all digital inputs and outputs
 * 
 * Manages:
 * - 3x4 Key Matrix (11 keys using 7 pins: 3 rows, 4 columns)
 * - Rotary Encoder (quadrature + optional button)
 * - Joystick Button
 * - Audio Input Switch
 * - Status LEDs (if implemented)
 */
class DigitalManager {
public:
    DigitalManager();
    ~DigitalManager();
    
    // Core lifecycle
    void Init(daisy::DaisySeed* hw);
    void Update();
    void Shutdown();
    
    // Health monitoring
    bool IsHealthy() const { return healthy_; }
    
    // Key Matrix
    bool IsKeyPressed(int row, int col) const;
    bool WasKeyPressed(int row, int col) const;
    bool IsKeyHeld(int row, int col) const;
    uint32_t GetKeyHoldTime(int row, int col) const;
    const KeyMatrixState& GetKeyMatrix() const;
    
    // Encoder
    int GetEncoderValue() const { return encoder_state_.value; }
    float GetEncoderDelta() const { return encoder_state_.delta; }
    bool IsEncoderButtonPressed() const { return encoder_state_.button_pressed; }
    bool WasEncoderButtonPressed() const { return encoder_state_.button_was_pressed; }
    uint32_t GetEncoderButtonHoldTime() const { return encoder_state_.button_hold_time; }
    
    // Joystick Button
    bool IsJoystickButtonPressed() const { return joystick_button_.pressed; }
    bool WasJoystickButtonPressed() const { return joystick_button_.was_pressed; }
    uint32_t GetJoystickButtonHoldTime() const { return joystick_button_.hold_time; }
    
    // Audio Switch
    bool IsAudioInputSwitched() const { return audio_switch_.pressed; }
    bool WasAudioInputSwitched() const { return audio_switch_.was_pressed; }
    
    // LEDs (if implemented)
    void SetLED(int led, bool state);
    void SetLEDBrightness(int led, float brightness);
    bool GetLED(int led) const;
    float GetLEDBrightness(int led) const;
    
    // Utility methods
    void ResetEncoder(int value = 0);
    void SetKeyMatrixDebounceTime(uint32_t ms);
    void SetButtonHoldThreshold(uint32_t ms);

private:
    // Hardware reference
    daisy::DaisySeed* hw_;
    
    // Pin assignments (based on pinout.md)
    daisy::Pin key_matrix_row_pins_[KEY_MATRIX_ROWS];
    daisy::Pin key_matrix_col_pins_[KEY_MATRIX_COLS];
    daisy::Pin encoder_a_pin_;
    daisy::Pin encoder_b_pin_;
    daisy::Pin encoder_button_pin_;
    daisy::Pin joystick_button_pin_;
    daisy::Pin audio_switch_pin_;
    daisy::Pin led_pins_[4]; // 4 status LEDs if implemented
    
    // GPIO objects
    daisy::GPIO key_matrix_rows_[KEY_MATRIX_ROWS];
    daisy::GPIO key_matrix_cols_[KEY_MATRIX_COLS];
    daisy::GPIO joystick_button_gpio_;
    daisy::GPIO audio_switch_gpio_;
    daisy::GPIO led_gpios_[4];
    
    // Daisy encoder object
    daisy::Encoder encoder_;
    
    // State tracking
    KeyMatrixState key_matrix_;
    EncoderState encoder_state_;
    ButtonState joystick_button_;
    ButtonState audio_switch_;
    bool led_states_[4];
    float led_brightness_[4];
    
    // Configuration
    uint32_t debounce_time_ms_;
    uint32_t hold_threshold_ms_;
    bool healthy_;
    
    // Internal methods
    void UpdateKeyMatrix();
    void UpdateEncoder();
    void UpdateButtons();
    void UpdateLEDs();
    void ScanKeyMatrix();
    bool IsValidKeyPosition(int row, int col) const;
    void UpdateButtonState(ButtonState& button, bool current_pressed);
};
