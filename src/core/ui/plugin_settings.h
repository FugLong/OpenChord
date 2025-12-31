#pragma once

#include <cstdint>

namespace OpenChord {

/**
 * Plugin Setting Types
 */
enum class SettingType {
    INT,      // Integer value
    FLOAT,    // Floating point value
    BOOL,     // Boolean value
    ENUM,     // Enumeration (string options)
    STRING    // String value
};

/**
 * Plugin Setting Definition
 * 
 * Describes a single configurable parameter for a plugin
 */
struct PluginSetting {
    const char* name;              // Display name (e.g., "Key", "Mode")
    SettingType type;              // Type of setting
    void* value_ptr;               // Pointer to actual value (must remain valid)
    float min_value;               // Minimum value (for INT/FLOAT)
    float max_value;               // Maximum value (for INT/FLOAT)
    float step_size;               // Step size for changes (for INT/FLOAT, default 1.0 or 0.1)
    const char** enum_options;     // For ENUM type (null-terminated array of strings)
    int enum_count;                // Number of enum options (if enum_options is provided)
    void (*on_change_callback)(void* value); // Optional callback when value changes
    
    // Convenience constructors would be nice but keeping it simple for C++11 compatibility
};

/**
 * Interface for plugins that expose settings
 * 
 * Plugins implementing this interface can have their settings
 * displayed and edited through the Settings Manager UI
 */
class IPluginWithSettings {
public:
    virtual ~IPluginWithSettings() = default;
    
    // Get number of settings this plugin exposes
    virtual int GetSettingCount() const = 0;
    
    // Get a setting by index (0 to GetSettingCount()-1)
    // Returns nullptr if index is invalid
    virtual const PluginSetting* GetSetting(int index) const = 0;
    
    // Optional: Custom settings UI rendering
    // If implemented, Settings Manager will call this instead of auto-generating UI
    // Parameters: display, selected_index (which setting is highlighted)
    // Return true if custom rendering was used, false to use auto-generated UI
    virtual bool RenderSettingsUI(void* display, int selected_index) {
        (void)display;
        (void)selected_index;
        return false;  // Default: use auto-generated UI
    }
    
    // Called when a setting value changes (if PluginSetting.on_change_callback is nullptr)
    // This is a fallback notification - plugins can use on_change_callback for immediate updates
    virtual void OnSettingChanged(int setting_index) {
        (void)setting_index;
        // Default: no-op
    }
};

} // namespace OpenChord
