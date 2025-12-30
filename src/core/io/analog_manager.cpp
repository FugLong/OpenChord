#include "analog_manager.h"
#include "pin_config.h"
#include "daisy_seed.h"
#include <cstring>
#include <cmath>

AnalogManager::AnalogManager() 
    : hw_(nullptr), filter_strength_(0.1f), dead_zone_(0.05f), 
      battery_check_ms_(1000), low_battery_threshold_(3.0f), healthy_(true) {
    
    // Initialize ADC configuration
    memset(adc_configured_, 0, sizeof(adc_configured_));
    
    // Initialize analog inputs
    memset(inputs_, 0, sizeof(inputs_));
    
    // Initialize joystick calibration
    memset(&joystick_cal_, 0, sizeof(joystick_cal_));
    
    // Initialize battery state
    memset(&battery_, 0, sizeof(battery_));
    battery_.check_interval_ms = battery_check_ms_;
}

AnalogManager::~AnalogManager() {
    Shutdown();
}

void AnalogManager::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    
    // Initialize pin assignments
    // Using Daisy pin constants directly to avoid linker issues with static constexpr
    adc_pins_[0] = daisy::seed::A0;    // ADC0 - Volume pot (Pin 22, A0)
    adc_pins_[1] = daisy::seed::A1;    // ADC1 - Microphone (Pin 23, A1)
    adc_pins_[2] = daisy::seed::A2;    // ADC2 - Joystick X (Pin 24, A2, left/right)
    adc_pins_[3] = daisy::seed::A3;    // ADC3 - Joystick Y (Pin 25, A3, up/down)
    adc_pins_[4] = daisy::seed::A4;    // ADC4 - Battery monitor (Pin 26, A4)
    
    // Initialize joystick calibration
    joystick_cal_.center_x = 0.5f;
    joystick_cal_.center_y = 0.5f;
    joystick_cal_.dead_zone_x = dead_zone_;
    joystick_cal_.dead_zone_y = dead_zone_;
    joystick_cal_.max_x = 1.0f;
    joystick_cal_.max_y = 1.0f;
    joystick_cal_.min_x = 0.0f;
    joystick_cal_.min_y = 0.0f;
    joystick_cal_.calibrated = false;
    
    // Initialize battery monitoring
    battery_.voltage = 4.2f; // Assume full battery initially
    battery_.percentage = 100.0f;
    battery_.is_low = false;
    battery_.is_charging = false;
    battery_.last_check_time = 0;
    battery_.check_interval_ms = battery_check_ms_;
    
    healthy_ = true;
    
    // Configure ADC channels - AnalogManager now handles its own hardware
    ConfigureADC();
}

void AnalogManager::Update() {
    if (!hw_) return;
    
    UpdateADC();
    UpdateInputs();
    UpdateBattery();
    ApplyFiltering();
    ApplyCalibration();
    DetectClipping();
}

void AnalogManager::Shutdown() {
    if (!hw_) return;
    
    // Shutdown ADC if needed
    healthy_ = false;
    hw_ = nullptr;
}

// Volume control
float AnalogManager::GetVolume() const {
    return inputs_[0].filtered_value;
}

float AnalogManager::GetVolumeNormalized() const {
    return inputs_[0].normalized_value;
}

float AnalogManager::GetVolumeDelta() const {
    return inputs_[0].delta;
}

float AnalogManager::GetADCValue(int channel) const {
    if (channel >= 0 && channel < NUM_ADC_CHANNELS && inputs_[channel].healthy) {
        return inputs_[channel].raw_value;
    }
    return 0.0f;
}

// Joystick
void AnalogManager::GetJoystick(float* x, float* y) const {
    if (x) *x = inputs_[2].normalized_value;  // inputs_[2] = Joystick X (ADC channel 2)
    if (y) *y = inputs_[3].normalized_value;  // inputs_[3] = Joystick Y (ADC channel 3)
}

float AnalogManager::GetJoystickX() const {
    return inputs_[2].normalized_value;  // inputs_[2] = Joystick X (ADC channel 2)
}

float AnalogManager::GetJoystickY() const {
    return inputs_[3].normalized_value;  // inputs_[3] = Joystick Y (ADC channel 3)
}

float AnalogManager::GetJoystickDeltaX() const {
    return inputs_[2].delta;  // inputs_[2] = Joystick X (ADC channel 2)
}

float AnalogManager::GetJoystickDeltaY() const {
    return inputs_[3].delta;  // inputs_[3] = Joystick Y (ADC channel 3)
}

float AnalogManager::GetJoystickXRaw() const {
    return inputs_[2].raw_value;  // Raw ADC value (0.0-1.0) - inputs_[2] is ADC channel 2 (Joystick X)
}

float AnalogManager::GetJoystickYRaw() const {
    return inputs_[3].raw_value;  // Raw ADC value (0.0-1.0) - inputs_[3] is ADC channel 3 (Joystick Y)
}

// Microphone
float AnalogManager::GetMicrophoneLevel() const {
    return inputs_[4].filtered_value;  // inputs_[4] = ADC channel 4 (Microphone)
}

float AnalogManager::GetMicrophoneNormalized() const {
    return inputs_[4].normalized_value;  // inputs_[4] = ADC channel 4 (Microphone)
}

bool AnalogManager::IsMicrophoneClipping() const {
    return inputs_[4].clipping;  // inputs_[4] = ADC channel 4 (Microphone)
}

// Battery
float AnalogManager::GetBatteryVoltage() const {
    return battery_.voltage;
}

float AnalogManager::GetBatteryPercentage() const {
    return battery_.percentage;
}

bool AnalogManager::IsLowBattery() const {
    return battery_.is_low;
}

bool AnalogManager::IsBatteryCharging() const {
    return battery_.is_charging;
}

// Calibration
void AnalogManager::CalibrateJoystick() {
    if (!hw_) return;
    
    // Simple auto-calibration: find min/max values
    float min_x = 1.0f, max_x = 0.0f;
    float min_y = 1.0f, max_y = 0.0f;
    
    // Sample joystick for a short time to find range
    for (int i = 0; i < 100; i++) {
        float x = inputs_[2].raw_value;  // Joystick X (ADC channel 2)
        float y = inputs_[3].raw_value;  // Joystick Y (ADC channel 3)
        
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
        
        hw_->DelayMs(1);
    }
    
    // Update calibration
    joystick_cal_.min_x = min_x;
    joystick_cal_.max_x = max_x;
    joystick_cal_.min_y = min_y;
    joystick_cal_.max_y = max_y;
    joystick_cal_.center_x = (min_x + max_x) * 0.5f;
    joystick_cal_.center_y = (min_y + max_y) * 0.5f;
    joystick_cal_.calibrated = true;
}

void AnalogManager::CalibrateVolume() {
    // Volume pot doesn't need calibration, but we could add dead zone detection
    // For now, just mark as healthy
    inputs_[0].healthy = true;
}

void AnalogManager::ResetCalibration() {
    joystick_cal_.calibrated = false;
}

// Configuration
void AnalogManager::SetFilterStrength(float strength) {
    filter_strength_ = std::max(0.0f, std::min(1.0f, strength));
}

void AnalogManager::SetDeadZone(float dead_zone) {
    dead_zone_ = std::max(0.0f, std::min(0.5f, dead_zone));
    joystick_cal_.dead_zone_x = dead_zone_;
    joystick_cal_.dead_zone_y = dead_zone_;
}

void AnalogManager::SetBatteryCheckInterval(uint32_t ms) {
    battery_check_ms_ = ms;
    battery_.check_interval_ms = ms;
}

void AnalogManager::SetLowBatteryThreshold(float voltage) {
    low_battery_threshold_ = voltage;
}

// Private methods
void AnalogManager::ConfigureADC() {
    if (!hw_) return;
    
    // Reset ADC peripheral before configuration (helps with cold boot issues)
    hw_->DelayMs(10);
    
    // Configure volume pot, battery, joystick, and microphone ADC channels
    // Note: ADC channel numbers here are logical (array index), physical pins are different
    adc_configs_[0].InitSingle(adc_pins_[0]);  // ADC channel 0 - Volume pot (Pin 22, ADC0)
    adc_configs_[1].InitSingle(adc_pins_[4]);  // ADC channel 1 - Battery monitor (Pin 26, ADC4)
    adc_configs_[2].InitSingle(adc_pins_[2]);  // ADC channel 2 - Joystick X (Pin 24, ADC2, left/right)
    adc_configs_[3].InitSingle(adc_pins_[3]);  // ADC channel 3 - Joystick Y (Pin 25, ADC3, up/down)
    adc_configs_[4].InitSingle(adc_pins_[1]);  // ADC channel 4 - Microphone (Pin 23, ADC1)
    adc_configured_[0] = true;
    adc_configured_[1] = true;  // Battery
    adc_configured_[2] = true;  // Joystick X
    adc_configured_[3] = true;  // Joystick Y
    adc_configured_[4] = true;  // Microphone
    
    // Initialize the ADC system with 5 channels
    hw_->adc.Init(&adc_configs_[0], 5);
    
    // Add delay for ADC to stabilize
    hw_->DelayMs(20);
    
    hw_->adc.Start();
    
    // Mark configured inputs as healthy
    // Note: Array index matches ADC channel number, not physical input type
    // inputs_[0] = VOLUME_POT (ADC channel 0)
    // inputs_[1] = BATTERY_MON (ADC channel 1)
    // inputs_[2] = JOYSTICK_X (ADC channel 2)
    // inputs_[3] = JOYSTICK_Y (ADC channel 3)
    // inputs_[4] = MICROPHONE (ADC channel 4)
    inputs_[0].healthy = true;  // Volume
    inputs_[1].healthy = true;  // Battery
    inputs_[2].healthy = true;  // Joystick X
    inputs_[3].healthy = true;  // Joystick Y
    inputs_[4].healthy = true;  // Microphone
}

void AnalogManager::UpdateADC() {
    if (!hw_) return;
    
    // ADC is updated automatically by Daisy Seed
    // We just read the values when needed
}

void AnalogManager::UpdateInputs() {
    if (!hw_) return;
    
    // Read ADC values - array index matches ADC channel number
    // inputs_[0] = ADC channel 0 (Volume pot)
    float float_value = hw_->adc.GetFloat(0);
    inputs_[0].raw_value = float_value;
    inputs_[0].filtered_value = float_value;
    inputs_[0].healthy = (float_value >= 0.0f && float_value <= 1.0f);
    
    // inputs_[1] = ADC channel 1 (Battery monitor)
    float battery_value = hw_->adc.GetFloat(1);
    inputs_[1].raw_value = battery_value;
    inputs_[1].filtered_value = battery_value;
    inputs_[1].healthy = (battery_value >= 0.0f && battery_value <= 1.0f);
    
    // inputs_[2] = ADC channel 2 (Joystick X)
    float joystick_x = hw_->adc.GetFloat(2);
    inputs_[2].raw_value = joystick_x;
    inputs_[2].filtered_value = joystick_x;
    inputs_[2].healthy = (joystick_x >= 0.0f && joystick_x <= 1.0f);
    
    // inputs_[3] = ADC channel 3 (Joystick Y)
    float joystick_y = hw_->adc.GetFloat(3);
    inputs_[3].raw_value = joystick_y;
    inputs_[3].filtered_value = joystick_y;
    inputs_[3].healthy = (joystick_y >= 0.0f && joystick_y <= 1.0f);
    
    // inputs_[4] = ADC channel 4 (Microphone)
    float mic_value = hw_->adc.GetFloat(4);
    inputs_[4].raw_value = mic_value;
    inputs_[4].filtered_value = mic_value;
    inputs_[4].healthy = (mic_value >= 0.0f && mic_value <= 1.0f);
}

void AnalogManager::UpdateBattery() {
    if (!hw_) return;
    
    uint32_t current_time = hw_->system.GetNow();
    if (current_time - battery_.last_check_time >= battery_.check_interval_ms) {
        // Read battery voltage from ADC channel 1 -> inputs_[1]
        float adc_value = inputs_[1].filtered_value;
        
        // Convert ADC value to voltage (3.3V reference, 2:1 voltage divider)
        // 2:1 divider (equal resistors) gives division ratio of 0.5
        // Battery voltage = (adc_value * 3.3V) / 0.5 = adc_value * 6.6V
        battery_.voltage = adc_value * 6.6f;
        
        // Calculate percentage (assuming 3.0V to 4.2V range)
        battery_.percentage = CalculateBatteryPercentage(battery_.voltage);
        
        // Check if battery is low
        battery_.is_low = battery_.voltage < low_battery_threshold_;
        
        battery_.last_check_time = current_time;
    }
}

void AnalogManager::ApplyFiltering() {
    // Simple low-pass filter for all inputs
    for (int i = 0; i < NUM_ADC_CHANNELS; i++) {
        if (inputs_[i].healthy) {
            inputs_[i].filtered_value = inputs_[i].filtered_value * (1.0f - filter_strength_) + 
                                       inputs_[i].raw_value * filter_strength_;
        }
    }
}

void AnalogManager::ApplyCalibration() {
    // Always set normalized values, even without calibration
    // Convert from 0.0-1.0 range to -1.0 to 1.0 centered range
    
    if (inputs_[2].healthy) { // X axis (ADC channel 2)
        float x = inputs_[2].filtered_value;
        float normalized_x;
        
        if (joystick_cal_.calibrated) {
            // Use calibration
            normalized_x = (x - joystick_cal_.center_x) / (joystick_cal_.max_x - joystick_cal_.min_x);
            
            // Apply dead zone
            if (fabs(normalized_x) < joystick_cal_.dead_zone_x) {
                normalized_x = 0.0f;
            }
        } else {
            // Simple conversion: 0.0-1.0 -> -1.0 to 1.0 (center at 0.5)
            normalized_x = (x - 0.5f) * 2.0f;
            
            // Apply dead zone
            if (fabs(normalized_x) < dead_zone_) {
                normalized_x = 0.0f;
            }
        }
        
        inputs_[2].normalized_value = std::max(-1.0f, std::min(1.0f, normalized_x));
    }
    
    if (inputs_[3].healthy) { // Y axis (ADC channel 3)
        float y = inputs_[3].filtered_value;
        float normalized_y;
        
        if (joystick_cal_.calibrated) {
            // Use calibration
            normalized_y = (y - joystick_cal_.center_y) / (joystick_cal_.max_y - joystick_cal_.min_y);
            
            // Apply dead zone
            if (fabs(normalized_y) < joystick_cal_.dead_zone_y) {
                normalized_y = 0.0f;
            }
        } else {
            // Simple conversion: 0.0-1.0 -> -1.0 to 1.0 (center at 0.5)
            // Invert Y axis (typical joystick: up should be positive)
            normalized_y = (0.5f - y) * 2.0f;
            
            // Apply dead zone
            if (fabs(normalized_y) < dead_zone_) {
                normalized_y = 0.0f;
            }
        }
        
        inputs_[3].normalized_value = std::max(-1.0f, std::min(1.0f, normalized_y));
    }
}

void AnalogManager::DetectClipping() {
    // Check for clipping on microphone input
    if (inputs_[4].healthy) { // Microphone (ADC channel 4)
        inputs_[4].clipping = inputs_[4].filtered_value > 0.95f;
    }
}

float AnalogManager::NormalizeValue(float raw_value, AnalogInputType type) {
    switch (type) {
        case AnalogInputType::VOLUME_POT:
            return raw_value; // Already 0.0f to 1.0f
            
        case AnalogInputType::JOYSTICK_X:
        case AnalogInputType::JOYSTICK_Y:
            return raw_value; // Will be calibrated later
            
        case AnalogInputType::MICROPHONE:
            return raw_value; // Already 0.0f to 1.0f
            
        case AnalogInputType::BATTERY_MONITOR:
            return raw_value; // Will be converted to voltage later
            
        default:
            return raw_value;
    }
}

float AnalogManager::CalculateBatteryPercentage(float voltage) {
    // Simple linear interpolation from 3.0V (0%) to 4.2V (100%)
    if (voltage <= 3.0f) return 0.0f;
    if (voltage >= 4.2f) return 100.0f;
    
    return (voltage - 3.0f) / (4.2f - 3.0f) * 100.0f;
}

bool AnalogManager::IsValidADCValue(float value) const {
    // Check if ADC value is within reasonable range
    return value >= 0.0f && value <= 1.0f;
}
