#pragma once

#include "volume_interface.h"
#include "io.h"

namespace OpenChord {

/**
 * Volume Manager implementation
 * Handles all volume control logic with coordinated amplitude + line level approach
 */
class VolumeManager : public IVolumeManager {
public:
    VolumeManager();
    ~VolumeManager();
    
    // IVolumeManager interface implementation
    void Update() override;
    const VolumeData& GetVolumeData() const override;
    bool HasVolumeChanged() const override;
    void ClearChangeFlag() override;
    
    // Volume curve and setting methods
    void SetAmplitudeCurve(float exponent) override;
    void SetLineLevelCurve(float exponent) override;
    void SetInputScaling(float scale_factor) override;
    void SetDeadZone(float dead_zone) override;
    void SetMinThreshold(float min_threshold) override;
    
    // Set the IO reference (called by system during initialization)
    void SetIO(IO* io);
    
private:
    IO* io_;  // Reference to IO system for ADC access
    
    // Volume control parameters
    float amplitude_exponent_;
    float line_level_exponent_;
    float input_scale_factor_;
    float dead_zone_;
    float min_threshold_;
    
    // Current volume data
    VolumeData volume_data_;
    bool has_changed_;
};

} // namespace OpenChord
