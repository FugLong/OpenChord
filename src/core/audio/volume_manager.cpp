#include "../../include/volume_manager.h"
#include "../../include/io.h"
#include <cmath>

namespace OpenChord {

VolumeManager::VolumeManager() 
    : amplitude_exponent_(0.3f), line_level_exponent_(0.4f), 
      input_scale_factor_(1.0f / 0.968f), dead_zone_(0.005f), 
      min_threshold_(0.000001f), has_changed_(false) {
    // Initialize volume data
    volume_data_.raw_adc = 0.0f;
    volume_data_.scaled_volume = 0.0f;
    volume_data_.amplitude = 0.0f;
    volume_data_.line_level = 0.0f;
    volume_data_.has_changed = false;
}

VolumeManager::~VolumeManager() = default;

void VolumeManager::Update() {
    // Get current ADC value from IO system
    float current_raw = io_->GetADCValue(0); // Channel 0 for volume pot
    
    // Check if values have changed significantly
    if (fabs(current_raw - volume_data_.raw_adc) > 0.01f) {
        // Store previous values for change detection
        float prev_amplitude = volume_data_.amplitude;
        float prev_line_level = volume_data_.line_level;
        
        // Update raw ADC value
        volume_data_.raw_adc = current_raw;
        
        // Apply coordinated volume control (our working method)
        if (volume_data_.raw_adc < dead_zone_) {
            volume_data_.amplitude = 0.0f;
            volume_data_.line_level = 0.0f;
        } else {
            // Scale the input to compensate for pot's actual range
            volume_data_.scaled_volume = volume_data_.raw_adc * input_scale_factor_;
            
            // Coordinated control: amplitude for oscillator, line level for output scaling
            volume_data_.amplitude = powf(volume_data_.scaled_volume, amplitude_exponent_);
            volume_data_.line_level = powf(volume_data_.scaled_volume, line_level_exponent_);
            
            // Apply minimum thresholds for better low-volume control
            if (volume_data_.amplitude < min_threshold_ && volume_data_.amplitude > 0.0f) {
                volume_data_.amplitude = min_threshold_;
            }
            if (volume_data_.line_level < min_threshold_ && volume_data_.line_level > 0.0f) {
                volume_data_.line_level = min_threshold_;
            }
        }
        
        // Clamp to valid range
        if (volume_data_.amplitude > 1.0f) volume_data_.amplitude = 1.0f;
        if (volume_data_.amplitude < 0.0f) volume_data_.amplitude = 0.0f;
        if (volume_data_.line_level > 1.0f) volume_data_.line_level = 1.0f;
        if (volume_data_.line_level < 0.0f) volume_data_.line_level = 0.0f;
        
        // Check if final values changed
        if (fabs(volume_data_.amplitude - prev_amplitude) > 0.001f ||
            fabs(volume_data_.line_level - prev_line_level) > 0.001f) {
            has_changed_ = true;
            volume_data_.has_changed = true;
        }
    }
}

const VolumeData& VolumeManager::GetVolumeData() const {
    return volume_data_;
}

bool VolumeManager::HasVolumeChanged() const {
    return has_changed_;
}

void VolumeManager::ClearChangeFlag() {
    has_changed_ = false;
    volume_data_.has_changed = false;
}

// Volume curve and setting methods
void VolumeManager::SetAmplitudeCurve(float exponent) {
    amplitude_exponent_ = exponent;
}

void VolumeManager::SetLineLevelCurve(float exponent) {
    line_level_exponent_ = exponent;
}

void VolumeManager::SetInputScaling(float scale_factor) {
    input_scale_factor_ = scale_factor;
}

void VolumeManager::SetDeadZone(float dead_zone) {
    dead_zone_ = dead_zone;
}

void VolumeManager::SetMinThreshold(float min_threshold) {
    min_threshold_ = min_threshold;
}

// Set the IO reference (called by system during initialization)
void VolumeManager::SetIO(IO* io) {
    io_ = io;
}

} // namespace OpenChord
