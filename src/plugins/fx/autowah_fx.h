#pragma once

#include "../../core/plugin_interface.h"
#include "../../core/ui/plugin_settings.h"
#include "daisysp.h"
#include <cstddef>

namespace OpenChord {

/**
 * AutowahFX - Autowah effect plugin
 * 
 * Features:
 * - Wah amount (0-1)
 * - Level (0-1)
 * - Wet/Dry mix (0-100, but stored as 0-1)
 * - Bypass
 */
class AutowahFX : public IEffectPlugin, public IPluginWithSettings {
public:
    AutowahFX();
    ~AutowahFX();
    
    // IPlugin interface
    void Init() override;
    void Process(const float* const* in, float* const* out, size_t size) override;
    void Update() override;
    void UpdateUI() override;
    void HandleEncoder(int encoder, float delta) override;
    void HandleButton(int button, bool pressed) override;
    void HandleJoystick(float x, float y) override;
    
    const char* GetName() const override { return "Autowah"; }
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
    void UpdateAutowahParams();
    
    // Effect parameters
    float sample_rate_;
    float wah_;              // 0-1
    float level_;            // 0-1
    float wet_dry_;         // 0-1.0 (0=dry, 1=wet)
    bool bypassed_;
    
    // Settings storage
    float wah_setting_value_;
    float level_setting_value_;
    float wet_dry_setting_value_;
    bool bypassed_setting_value_;
    
    // Settings array
    PluginSetting settings_[4];
    
    // DaisySP autowah
    daisysp::Autowah autowah_;
    bool initialized_;
};

} // namespace OpenChord

