#pragma once

#include "daisy_seed.h"
#include <cstdint>

// ADC channel configuration
static constexpr int NUM_ADC_CHANNELS = 5;

// Analog input types
enum class AnalogInputType {
    VOLUME_POT,      // ADC0 - Pin 22
    JOYSTICK_X,      // ADC2 - Pin 24  
    JOYSTICK_Y,      // ADC3 - Pin 25
    MICROPHONE,      // ADC1 - Pin 23
    BATTERY_MONITOR  // ADC4 - Pin 26
};

// Analog input state
struct AnalogInputState {
    float raw_value;           // Raw ADC value (0.0f to 1.0f)
    float filtered_value;      // Filtered/smoothed value
    float normalized_value;     // Normalized value for UI
    float last_value;          // Previous value for delta calculations
    float delta;               // Change since last update
    bool clipping;             // Whether input is clipping
    bool healthy;              // Whether input is working properly
    uint32_t update_count;     // Number of updates
    uint32_t last_update_time; // Last update timestamp
};

// Joystick calibration data
struct JoystickCalibration {
    float center_x, center_y;      // Center position
    float dead_zone_x, dead_zone_y; // Dead zone radius
    float max_x, max_y;            // Maximum values
    float min_x, min_y;            // Minimum values
    bool calibrated;               // Whether calibration is complete
};

// Battery monitoring
struct BatteryState {
    float voltage;                 // Current voltage
    float percentage;              // Battery percentage (0-100%)
    bool is_low;                   // Low battery warning
    bool is_charging;              // Whether battery is charging
    uint32_t last_check_time;      // Last battery check
    uint32_t check_interval_ms;    // How often to check battery
};

/**
 * Analog Manager - Handles all analog inputs and processing
 * 
 * Manages:
 * - Volume potentiometer with normalization
 * - Joystick X/Y axes with calibration and dead zones
 * - Microphone input with clipping detection
 * - Battery voltage monitoring with percentage calculation
 * - ADC noise filtering and smoothing
 */
class AnalogManager {
public:
    AnalogManager();
    ~AnalogManager();
    
    // Core lifecycle
    void Init(daisy::DaisySeed* hw);
    void Update();
    void Shutdown();
    
    // Health monitoring
    bool IsHealthy() const { return healthy_; }
    
    // ADC configuration (handled internally during Init)
    
    // Volume control
    float GetVolume() const;
    float GetVolumeNormalized() const;
    float GetVolumeDelta() const;
    
    // Raw ADC access (for volume manager compatibility)
    float GetADCValue(int channel) const;
    
    // Joystick
    void GetJoystick(float* x, float* y) const;
    float GetJoystickX() const;
    float GetJoystickY() const;
    float GetJoystickDeltaX() const;
    float GetJoystickDeltaY() const;
    float GetJoystickXRaw() const;  // Raw ADC value (0.0-1.0)
    float GetJoystickYRaw() const;  // Raw ADC value (0.0-1.0)
    
    // Microphone
    float GetMicrophoneLevel() const;
    float GetMicrophoneNormalized() const;
    bool IsMicrophoneClipping() const;
    
    // Battery
    float GetBatteryVoltage() const;
    float GetBatteryPercentage() const;
    bool IsLowBattery() const;
    bool IsBatteryCharging() const;
    
    // Calibration
    void CalibrateJoystick();
    void CalibrateVolume();
    void ResetCalibration();
    const JoystickCalibration& GetJoystickCalibration() const { return joystick_cal_; }
    
    // Configuration
    void SetFilterStrength(float strength); // 0.0f = no filter, 1.0f = max filter
    void SetDeadZone(float dead_zone);      // Joystick dead zone (0.0f to 0.5f)
    void SetBatteryCheckInterval(uint32_t ms);
    void SetLowBatteryThreshold(float voltage);

private:
    // Hardware reference
    daisy::DaisySeed* hw_;
    
    // ADC configuration
    daisy::AdcChannelConfig adc_configs_[NUM_ADC_CHANNELS];
    bool adc_configured_[NUM_ADC_CHANNELS];
    
    // Pin assignments (based on pinout.md)
    daisy::Pin adc_pins_[NUM_ADC_CHANNELS];
    
    // Analog inputs
    AnalogInputState inputs_[NUM_ADC_CHANNELS];
    
    // Joystick calibration
    JoystickCalibration joystick_cal_;
    
    // Battery monitoring
    BatteryState battery_;
    
    // Configuration
    float filter_strength_;        // Low-pass filter strength
    float dead_zone_;             // Joystick dead zone
    uint32_t battery_check_ms_;   // Battery check interval
    float low_battery_threshold_; // Low battery voltage threshold
    bool healthy_;
    
    // Internal methods
    void ConfigureADC();
    void UpdateADC();
    void UpdateInputs();
    void UpdateBattery();
    void ApplyFiltering();
    void ApplyCalibration();
    void DetectClipping();
    float NormalizeValue(float raw_value, AnalogInputType type);
    float CalculateBatteryPercentage(float voltage);
    bool IsValidADCValue(float value) const;
};
