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
    
    // Initialize pin assignments using centralized pin configuration
    adc_pins_[0] = OpenChord::PinConfig::VOLUME_POT;    // ADC0 - Volume pot (Pin 22)
    adc_pins_[1] = OpenChord::PinConfig::MICROPHONE;    // ADC1 - Microphone (Pin 23)
    // Temporarily disable other ADC channels to get volume working first
    // adc_pins_[2] = OpenChord::PinConfig::JOYSTICK_X;    // ADC2 - Joystick X (Pin 24)
    // adc_pins_[3] = OpenChord::PinConfig::JOYSTICK_Y;    // ADC3 - Joystick Y (Pin 25)
    adc_pins_[4] = OpenChord::PinConfig::BATTERY_MON;   // ADC4 - Battery monitor (Pin 26)
    
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
    if (x) *x = inputs_[1].normalized_value;
    if (y) *y = inputs_[2].normalized_value;
}

float AnalogManager::GetJoystickX() const {
    return inputs_[1].normalized_value;
}

float AnalogManager::GetJoystickY() const {
    return inputs_[2].normalized_value;
}

float AnalogManager::GetJoystickDeltaX() const {
    return inputs_[1].delta;
}

float AnalogManager::GetJoystickDeltaY() const {
    return inputs_[2].delta;
}

// Microphone
float AnalogManager::GetMicrophoneLevel() const {
    return inputs_[3].filtered_value;
}

float AnalogManager::GetMicrophoneNormalized() const {
    return inputs_[3].normalized_value;
}

bool AnalogManager::IsMicrophoneClipping() const {
    return inputs_[3].clipping;
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
        float x = inputs_[1].raw_value;
        float y = inputs_[2].raw_value;
        
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
    
    // Configure volume pot, microphone, and battery ADC channels
    adc_configs_[0].InitSingle(adc_pins_[0]);  // ADC channel 0 - Volume pot
    adc_configs_[1].InitSingle(adc_pins_[1]);  // ADC channel 1 - Microphone
    adc_configs_[2].InitSingle(adc_pins_[4]);  // ADC channel 2 - Battery monitor (Pin 26, ADC4)
    adc_configured_[0] = true;
    adc_configured_[1] = true;
    adc_configured_[4] = true;  // Mark battery input as configured (uses inputs_[4])
    
    // Initialize the ADC system with three channels
    hw_->adc.Init(&adc_configs_[0], 3);
    
    // Add delay for ADC to stabilize
    hw_->DelayMs(20);
    
    hw_->adc.Start();
    
    // Mark configured inputs as healthy
    // inputs_[0] = VOLUME_POT (ADC channel 0)
    // inputs_[3] = MICROPHONE (ADC channel 1)
    // inputs_[4] = BATTERY_MON (ADC channel 2)
    inputs_[0].healthy = true;
    inputs_[3].healthy = true;
    inputs_[4].healthy = true;
    for (int i = 1; i < NUM_ADC_CHANNELS; i++) {
        if (i != 3 && i != 4) {
            inputs_[i].healthy = false;
        }
    }
}

void AnalogManager::UpdateADC() {
    if (!hw_) return;
    
    // ADC is updated automatically by Daisy Seed
    // We just read the values when needed
}

void AnalogManager::UpdateInputs() {
    if (!hw_) return;
    
    // Read the volume pot value (ADC channel 0 -> inputs_[0])
    float float_value = hw_->adc.GetFloat(0);
    
    // Store the float value
    inputs_[0].raw_value = float_value;
    inputs_[0].filtered_value = float_value;
    inputs_[0].healthy = (float_value >= 0.0f && float_value <= 1.0f);
    
    // Read the microphone value (ADC channel 1 -> inputs_[3])
    float mic_value = hw_->adc.GetFloat(1);
    
    // Store the microphone value (MAX9814 outputs centered around ~0.38, range ~0.1-0.65)
    inputs_[3].raw_value = mic_value;
    inputs_[3].filtered_value = mic_value;
    inputs_[3].healthy = (mic_value >= 0.0f && mic_value <= 1.0f);
    
    // Read the battery voltage (ADC channel 2 -> inputs_[4])
    float battery_value = hw_->adc.GetFloat(2);
    inputs_[4].raw_value = battery_value;
    inputs_[4].filtered_value = battery_value;
    inputs_[4].healthy = (battery_value >= 0.0f && battery_value <= 1.0f);
    
    // Commented out verbose debug logging
    // static uint32_t debug_counter = 0;
    // if (++debug_counter % 100 == 0) { // Every 100 iterations = ~1 second
    //     hw_->PrintLine("UpdateInputs Debug - ADC: %.4f, Stored: %.4f", float_value, inputs_[0].raw_value);
    // }
    
    // Mark other unused channels as unhealthy
    for (int i = 1; i < NUM_ADC_CHANNELS; i++) {
        if (i != 3 && i != 4) {
            inputs_[i].raw_value = 0.0f;
            inputs_[i].filtered_value = 0.0f;
            inputs_[i].healthy = false;
        }
    }
}

void AnalogManager::UpdateBattery() {
    if (!hw_) return;
    
    uint32_t current_time = hw_->system.GetNow();
    if (current_time - battery_.last_check_time >= battery_.check_interval_ms) {
        // Read battery voltage from ADC
        float adc_value = inputs_[4].filtered_value;
        
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
    if (!joystick_cal_.calibrated) return;
    
    // Apply joystick calibration
    if (inputs_[1].healthy) { // X axis
        float x = inputs_[1].filtered_value;
        float normalized_x = (x - joystick_cal_.center_x) / (joystick_cal_.max_x - joystick_cal_.min_x);
        
        // Apply dead zone
        if (fabs(normalized_x) < joystick_cal_.dead_zone_x) {
            normalized_x = 0.0f;
        }
        
        inputs_[1].normalized_value = std::max(-1.0f, std::min(1.0f, normalized_x));
    }
    
    if (inputs_[2].healthy) { // Y axis
        float y = inputs_[2].filtered_value;
        float normalized_y = (y - joystick_cal_.center_y) / (joystick_cal_.max_y - joystick_cal_.min_y);
        
        // Apply dead zone
        if (fabs(normalized_y) < joystick_cal_.dead_zone_y) {
            normalized_y = 0.0f;
        }
        
        inputs_[2].normalized_value = std::max(-1.0f, std::min(1.0f, normalized_y));
    }
}

void AnalogManager::DetectClipping() {
    // Check for clipping on microphone input
    if (inputs_[3].healthy) {
        inputs_[3].clipping = inputs_[3].filtered_value > 0.95f;
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
