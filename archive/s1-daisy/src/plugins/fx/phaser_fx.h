#pragma once

#include "../../core/plugin_interface.h"
#include "../../core/ui/plugin_settings.h"
#include "daisysp.h"
#include <cstddef>

namespace OpenChord {

/**
 * PhaserFX - Phaser effect plugin
 * 
 * Features:
 * - LFO Depth (0-1)
 * - LFO Frequency (0.1-10 Hz)
 * - Allpass Frequency (100-10000 Hz)
 * - Feedback (0-1)
 * - Poles (1-8)
 * - Wet/Dry mix
 * - Bypass
 */
class PhaserFX : public IEffectPlugin, public IPluginWithSettings {
public:
    PhaserFX();
    ~PhaserFX();
    
    // IPlugin interface
    void Init() override;
    void Process(const float* const* in, float* const* out, size_t size) override;
    void Update() override;
    void UpdateUI() override;
    void HandleEncoder(int encoder, float delta) override;
    void HandleButton(int button, bool pressed) override;
    void HandleJoystick(float x, float y) override;
    
    const char* GetName() const override { return "Phaser"; }
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
    void UpdatePhaserParams();
    
    // Effect parameters
    float sample_rate_;
    float lfo_depth_;        // 0-1
    float lfo_freq_;         // 0.1-10 Hz
    float ap_freq_;          // 100-10000 Hz (allpass frequency)
    float feedback_;          // 0-1
    int poles_;              // 1-8
    float wet_dry_;         // 0-1.0 (0=dry, 1=wet)
    bool bypassed_;
    
    // Settings storage
    float lfo_depth_setting_value_;
    float lfo_freq_setting_value_;
    float ap_freq_setting_value_;
    float feedback_setting_value_;
    float poles_setting_value_;  // Stored as float for settings, converted to int
    float wet_dry_setting_value_;
    bool bypassed_setting_value_;
    
    // Settings array
    PluginSetting settings_[7];
    
    // DaisySP phaser
    daisysp::Phaser phaser_;
    bool initialized_;
};

} // namespace OpenChord

