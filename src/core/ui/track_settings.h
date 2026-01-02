#pragma once

#include "plugin_settings.h"
#include "../music/chord_engine.h"

namespace OpenChord {

class Track;

/**
 * Track Settings - Track-level settings (key, BPM, etc.)
 * 
 * Implements IPluginWithSettings interface so it works with SettingsManager
 * This allows track settings to use the same UI system as plugin settings
 */
class TrackSettings : public IPluginWithSettings {
public:
    TrackSettings();
    ~TrackSettings();
    
    // Set track to manage settings for
    void SetTrack(Track* track) { track_ = track; }
    
    // IPluginWithSettings interface
    int GetSettingCount() const override;
    const PluginSetting* GetSetting(int index) const override;
    void OnSettingChanged(int setting_index) override;
    
private:
    Track* track_;
    
    // Settings values (helpers for UI) - mutable so they can be updated in const methods
    mutable int key_root_value_;  // 0-11 (C through B)
    mutable int key_mode_value_;  // 0-6 (Ionian through Locrian)
    
    // Settings array
    static constexpr int SETTING_COUNT = 2;  // Key Root, Mode
    mutable PluginSetting settings_[SETTING_COUNT];
    
    // Initialize settings array
    void InitializeSettings();
    
    // Sync values with track key
    void SyncFromTrack() const;  // Const because it only updates mutable cache values
    void SyncToTrack();
};

} // namespace OpenChord

