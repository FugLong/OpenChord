#pragma once

#include "../../core/plugin_interface.h"
#include "../../core/ui/plugin_settings.h"
#include "daisysp.h"
#include <cstddef>

namespace OpenChord {

/**
 * OverdriveFX - Overdrive/distortion effect plugin
 * 
 * Features:
 * - Drive (0-1)
 * - Wet/Dry mix
 * - Bypass
 */
class OverdriveFX : public IEffectPlugin, public IPluginWithSettings {
public:
    OverdriveFX();
    ~OverdriveFX();
    
    // IPlugin interface
    void Init() override;
    void Process(const float* const* in, float* const* out, size_t size) override;
    void Update() override;
    void UpdateUI() override;
    void HandleEncoder(int encoder, float delta) override;
    void HandleButton(int button, bool pressed) override;
    void HandleJoystick(float x, float y) override;
    
    const char* GetName() const override { return "Overdrive"; }
    const char* GetCategory() const override { return "FX"; }
    int GetVersion() const override { return 1; }
    
    void SaveState(void* buffer, size_t* size) const override;
    void LoadState(const void* buffer, size_t size) override;
    size_t GetStateSize() const override;
    
    // IEffectPlugin interface
    void SetSampleRate(float sample_rate) override;
    void SetBypass(bool bypass) override;
    bool IsBypassed() const override { return bypassed_; }
    void SetWetDry(float wet_dry) override;
    float GetWetDry() const override { return wet_dry_; }
    
    // IPluginWithSettings interface
    int GetSettingCount() const override;
    const PluginSetting* GetSetting(int index) const override;
    void OnSettingChanged(int setting_index) override;
    
private:
    void InitializeSettings();
    void UpdateOverdriveParams();
    
    // Effect parameters
    float sample_rate_;
    float drive_;           // 0-1
    float wet_dry_;        // 0-1.0 (0=dry, 1=wet)
    bool bypassed_;
    
    // Settings storage
    float drive_setting_value_;
    float wet_dry_setting_value_;
    bool bypassed_setting_value_;
    
    // Settings array
    PluginSetting settings_[3];
    
    // DaisySP overdrive
    daisysp::Overdrive overdrive_;
    bool initialized_;
};

} // namespace OpenChord

