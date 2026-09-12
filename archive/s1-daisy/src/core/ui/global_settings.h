#pragma once

#include "plugin_settings.h"

namespace OpenChord {

/**
 * Transport Routing Options
 * Controls where play/pause/record commands are sent
 */
enum class TransportRouting {
    INTERNAL_ONLY = 0,  // Internal looper only
    DAW_ONLY = 1,       // DAW (MIDI output) only
    BOTH = 2            // Both internal looper and DAW
};

/**
 * Global Settings - Device-wide settings
 * 
 * Implements IPluginWithSettings interface so it works with SettingsManager
 * This allows global settings to use the same UI system as plugin settings
 */
class GlobalSettings : public IPluginWithSettings {
public:
    GlobalSettings();
    ~GlobalSettings();
    
    // IPluginWithSettings interface
    int GetSettingCount() const override;
    const PluginSetting* GetSetting(int index) const override;
    void OnSettingChanged(int setting_index) override;
    
    // Access to settings values
    TransportRouting GetTransportRouting() const { return transport_routing_; }
    void SetTransportRouting(TransportRouting routing) {
        transport_routing_ = routing;
        transport_routing_value_ = static_cast<int>(routing);
    }
    
private:
    // Settings values
    TransportRouting transport_routing_;
    int transport_routing_value_;  // Helper int for settings (synced with transport_routing_)
    
    // Settings array (similar to plugin pattern)
    static constexpr int SETTING_COUNT = 1;
    mutable PluginSetting settings_[SETTING_COUNT];
    
    // Initialize settings array
    void InitializeSettings();
    
    // Sync helper value with enum
    void SyncTransportRoutingValue();
};

} // namespace OpenChord

