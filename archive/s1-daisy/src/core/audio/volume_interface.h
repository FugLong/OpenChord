#pragma once

#include <cstdint>

namespace OpenChord {

/**
 * Volume data structure that can be accessed globally
 */
struct VolumeData {
    float raw_adc;           // Raw ADC value (0.0f - 1.0f)
    float scaled_volume;     // Scaled to compensate for pot range
    float amplitude;         // Oscillator amplitude control
    float line_level;        // Line output level control
    bool has_changed;        // Flag for change detection
};

/**
 * Interface for the global volume management system
 */
class IVolumeManager {
public:
    virtual ~IVolumeManager() = default;
    
    // Volume control
    virtual void Update() = 0;
    virtual const VolumeData& GetVolumeData() const = 0;
    virtual bool HasVolumeChanged() const = 0;
    virtual void ClearChangeFlag() = 0;
    
    // Volume curves and settings
    virtual void SetAmplitudeCurve(float exponent) = 0;
    virtual void SetLineLevelCurve(float exponent) = 0;
    virtual void SetInputScaling(float scale_factor) = 0;
    virtual void SetDeadZone(float dead_zone) = 0;
    virtual void SetMinThreshold(float min_threshold) = 0;
};

} // namespace OpenChord
