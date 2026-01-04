#include "settings_manager.h"
#include "plugin_settings.h"
#include <cstring>
#include <cstdio>
#include "daisy_seed.h"  // For Font_6x8

namespace OpenChord {

SettingsManager::SettingsManager()
    : display_(nullptr)
    , current_plugin_(nullptr)
    , selected_index_(0)
    , setting_count_(0)
{
}

SettingsManager::~SettingsManager() {
}

void SettingsManager::Init(DisplayManager* display) {
    display_ = display;
    current_plugin_ = nullptr;
    selected_index_ = 0;
    setting_count_ = 0;
}

void SettingsManager::SetPlugin(IPluginWithSettings* plugin) {
    current_plugin_ = plugin;
    selected_index_ = 0;
    
    if (plugin) {
        // Validate plugin by checking if it has settings
        // GetSettingCount() should return the number of settings (0 if not implemented)
        setting_count_ = plugin->GetSettingCount();
        
        // Clamp selected index
        if (selected_index_ >= setting_count_) {
            selected_index_ = setting_count_ > 0 ? setting_count_ - 1 : 0;
        }
        
        // Note: We trust GetSettingCount() - if it returns > 0, the plugin implements the interface
        // If there's a casting issue, GetSettingCount() will likely crash or return wrong value,
        // but we can't safely validate without RTTI. Rendering will handle invalid settings gracefully.
    } else {
        setting_count_ = 0;
    }
}

void SettingsManager::SetSelectedIndex(int index) {
    if (!current_plugin_) return;
    
    if (index < 0) {
        selected_index_ = 0;
    } else if (index >= setting_count_) {
        selected_index_ = setting_count_ > 0 ? setting_count_ - 1 : 0;
    } else {
        selected_index_ = index;
    }
}

void SettingsManager::MoveSelection(int direction) {
    SetSelectedIndex(selected_index_ + direction);
}

void SettingsManager::ChangeValue(float delta) {
    if (!current_plugin_ || selected_index_ < 0 || selected_index_ >= setting_count_) {
        return;
    }
    
    const PluginSetting* setting = current_plugin_->GetSetting(selected_index_);
    if (!setting || !setting->value_ptr) {
        return;
    }
    
    // Normal scrolling: 1 tick = 1 step (delta ~1.0, no scaling)
    // Fast scrolling: apply range-based acceleration multiplier on top of velocity acceleration
    // This allows large ranges to accelerate more when scrolling fast
    float abs_delta = std::abs(delta);
    
    // Check if this is fast scrolling (velocity acceleration already applied by encoder handler)
    // Normal scrolling: abs_delta ~1.0, fast scrolling: abs_delta >= 1.5 (velocity multiplier applied)
    // Use threshold of 1.5 to clearly distinguish fast scrolling from normal
    if (abs_delta >= 1.5f) {
        // Fast scrolling detected - apply range-based acceleration multiplier
        float range = setting->max_value - setting->min_value;
        if (range > 0.0f) {
            // Calculate range-based acceleration multiplier
            // For small ranges (0-1): multiplier stays close to 1.0 (minimal extra boost)
            // For large ranges (0-5000): multiplier can go up to ~5x
            // Formula: 1.0 + (range / 2000.0) gives:
            // - Range 0-1: multiplier = 1.0 (no extra boost)
            // - Range 0-5000: multiplier = 3.5 (significant boost for large ranges)
            float range_multiplier = 1.0f + (range / 2000.0f);
            if (range_multiplier > 5.0f) range_multiplier = 5.0f;  // Cap at 5x
            
            // Apply range-based acceleration on top of velocity acceleration
            // Preserve direction
            if (delta > 0.0f) {
                delta = abs_delta * range_multiplier;
            } else {
                delta = -abs_delta * range_multiplier;
            }
        }
    }
    // For normal scrolling (abs_delta < 1.5), use as-is (1:1 mapping, no range scaling)
    
    UpdateValue(selected_index_, delta);
}

void SettingsManager::ToggleValue() {
    if (!current_plugin_ || selected_index_ < 0 || selected_index_ >= setting_count_) {
        return;
    }
    
    const PluginSetting* setting = current_plugin_->GetSetting(selected_index_);
    if (!setting || !setting->value_ptr) {
        return;
    }
    
    if (setting->type == SettingType::BOOL) {
        // Toggle boolean
        bool* bool_val = static_cast<bool*>(setting->value_ptr);
        *bool_val = !(*bool_val);
        
        // Call callback if provided
        if (setting->on_change_callback) {
            setting->on_change_callback(setting->value_ptr);
        } else {
            current_plugin_->OnSettingChanged(selected_index_);
        }
    } else if (setting->type == SettingType::ENUM) {
        // Cycle enum value
        CycleEnumValue(selected_index_);
    }
}

void SettingsManager::UpdateValue(int setting_index, float delta) {
    const PluginSetting* setting = current_plugin_->GetSetting(setting_index);
    if (!setting || !setting->value_ptr) {
        return;
    }
    
    // Get step size (default based on type)
    float step = setting->step_size > 0.0f ? setting->step_size : 
                 (setting->type == SettingType::INT ? 1.0f : 0.1f);
    
    float change = delta * step;
    
    switch (setting->type) {
        case SettingType::INT: {
            int* int_val = static_cast<int*>(setting->value_ptr);
            int new_val = *int_val + static_cast<int>(change);
            // Clamp to min/max
            if (setting->min_value != setting->max_value) {
                if (new_val < static_cast<int>(setting->min_value)) {
                    new_val = static_cast<int>(setting->min_value);
                } else if (new_val > static_cast<int>(setting->max_value)) {
                    new_val = static_cast<int>(setting->max_value);
                }
            }
            *int_val = new_val;
            break;
        }
        
        case SettingType::FLOAT: {
            float* float_val = static_cast<float*>(setting->value_ptr);
            float new_val = *float_val + change;
            // Clamp to min/max
            if (setting->min_value != setting->max_value) {
                if (new_val < setting->min_value) {
                    new_val = setting->min_value;
                } else if (new_val > setting->max_value) {
                    new_val = setting->max_value;
                }
            }
            *float_val = new_val;
            break;
        }
        
        case SettingType::ENUM: {
            // For enums, delta > 0 goes forward, delta < 0 goes backward
            int* enum_val = static_cast<int*>(setting->value_ptr);
            if (change > 0) {
                CycleEnumValue(setting_index);
            } else if (change < 0) {
                // Cycle backward
                (*enum_val)--;
                if (*enum_val < 0) {
                    *enum_val = setting->enum_count - 1;
                }
                // Call callback if provided
                if (setting->on_change_callback) {
                    setting->on_change_callback(setting->value_ptr);
                } else {
                    current_plugin_->OnSettingChanged(setting_index);
                }
            }
            break;
        }
        
        case SettingType::BOOL:
            // Bool handled by ToggleValue()
            break;
            
        case SettingType::STRING:
            // Strings not editable via encoder
            break;
    }
    
    // Call callback if provided (and not already called above)
    if (setting->type != SettingType::ENUM && setting->on_change_callback) {
        setting->on_change_callback(setting->value_ptr);
    } else if (setting->type != SettingType::ENUM) {
        current_plugin_->OnSettingChanged(setting_index);
    }
}

void SettingsManager::CycleEnumValue(int setting_index) {
    const PluginSetting* setting = current_plugin_->GetSetting(setting_index);
    if (!setting || !setting->value_ptr || !setting->enum_options) {
        return;
    }
    
    int* enum_val = static_cast<int*>(setting->value_ptr);
    (*enum_val)++;
    if (*enum_val >= setting->enum_count) {
        *enum_val = 0;
    }
    
    // Call callback if provided
    if (setting->on_change_callback) {
        setting->on_change_callback(setting->value_ptr);
    } else {
        current_plugin_->OnSettingChanged(setting_index);
    }
}

void SettingsManager::Render() {
    if (!IsHealthy() || !current_plugin_) {
        return;
    }
    
    auto* disp = display_->GetDisplay();
    if (!disp) return;
    
    // Try custom rendering first
    if (current_plugin_->RenderSettingsUI(disp, selected_index_)) {
        return;  // Plugin handled rendering
    }
    
    // Otherwise use auto-generated UI
    RenderAutoGeneratedUI();
}

void SettingsManager::RenderAutoGeneratedUI() {
    auto* disp = display_->GetDisplay();
    if (!disp || !current_plugin_) return;
    
    // Validate: Check if we actually have settings to display
    if (setting_count_ <= 0) {
        // No settings - display a message
        disp->SetCursor(0, 10);
        disp->WriteString("No settings available", Font_6x8, true);
        return;
    }
    
    // Content area starts at y=10 (below system bar)
    int y = 10;
    const int line_height = 10;
    const int max_visible_lines = 5;  // Roughly how many settings fit on screen
    
    // Calculate which settings to display
    int start_index = 0;
    if (selected_index_ >= max_visible_lines) {
        start_index = selected_index_ - max_visible_lines + 1;
    }
    
    int end_index = start_index + max_visible_lines;
    if (end_index > setting_count_) {
        end_index = setting_count_;
    }
    
    // Calculate content area bounds for scrollbar
    const int content_start_y = y;
    const int content_height = 64 - content_start_y;  // Screen height (64) minus content start
    const int scrollbar_x = 126;  // Right edge (2 pixels wide: 126 and 127)
    
    // Render scrollbar if needed
    if (setting_count_ > max_visible_lines) {
        // Calculate scrollbar thumb position and size
        float scrollbar_track_height = static_cast<float>(content_height);
        float visible_ratio = static_cast<float>(max_visible_lines) / static_cast<float>(setting_count_);
        float thumb_height = scrollbar_track_height * visible_ratio;
        if (thumb_height < 4.0f) thumb_height = 4.0f;  // Minimum thumb size
        
        float scroll_ratio = static_cast<float>(start_index) / static_cast<float>(setting_count_ - max_visible_lines);
        if (setting_count_ <= max_visible_lines) scroll_ratio = 0.0f;
        
        int thumb_start_y = content_start_y + static_cast<int>(scroll_ratio * (scrollbar_track_height - thumb_height));
        int thumb_end_y = thumb_start_y + static_cast<int>(thumb_height);
        if (thumb_end_y > 64) thumb_end_y = 64;
        if (thumb_start_y < content_start_y) thumb_start_y = content_start_y;
        
        // Draw scrollbar track (subtle - just outline edges)
        disp->DrawPixel(scrollbar_x, content_start_y, true);  // Top edge
        disp->DrawPixel(scrollbar_x + 1, content_start_y, true);
        disp->DrawPixel(scrollbar_x, 63, true);  // Bottom edge (64-1)
        disp->DrawPixel(scrollbar_x + 1, 63, true);
        
        // Draw scrollbar thumb (solid filled rectangle - this is what shows position)
        for (int sy = thumb_start_y; sy < thumb_end_y; sy++) {
            disp->DrawPixel(scrollbar_x, sy, true);
            disp->DrawPixel(scrollbar_x + 1, sy, true);
        }
    }
    
    // Render visible settings
    int visible_idx = 0;
    for (int i = start_index; i < end_index; i++) {
        bool is_selected = (i == selected_index_);
        
        // Calculate Y position for this visible item (accounting for start_index offset)
        int item_y = 10 + (visible_idx * line_height);
        if (item_y >= 64) break;  // Screen is 64 pixels tall
        
        // Render the setting item
        RenderSettingItem(i, is_selected, item_y);
        
        visible_idx++;
    }
}

void SettingsManager::RenderSettingItem(int index, bool is_selected, int y_pos) {
    auto* disp = display_->GetDisplay();
    if (!disp || !current_plugin_) return;
    
    // Validate index
    if (index < 0 || index >= setting_count_) {
        return;
    }
    
    const PluginSetting* setting = current_plugin_->GetSetting(index);
    if (!setting || !setting->name) {
        // Invalid setting - skip
        return;
    }
    
    char buffer[64];
    const char* prefix = is_selected ? "> " : "  ";
    
    // Format value based on type
    const char* value_str = "";
    switch (setting->type) {
        case SettingType::INT: {
            int* int_val = static_cast<int*>(setting->value_ptr);
            snprintf(buffer, sizeof(buffer), "%s%s: %d", prefix, setting->name, *int_val);
            value_str = buffer;
            break;
        }
        
        case SettingType::FLOAT: {
            float* float_val = static_cast<float*>(setting->value_ptr);
            snprintf(buffer, sizeof(buffer), "%s%s: %.2f", prefix, setting->name, *float_val);
            value_str = buffer;
            break;
        }
        
        case SettingType::BOOL: {
            bool* bool_val = static_cast<bool*>(setting->value_ptr);
            snprintf(buffer, sizeof(buffer), "%s%s: %s", prefix, setting->name, *bool_val ? "ON" : "OFF");
            value_str = buffer;
            break;
        }
        
        case SettingType::ENUM: {
            // For enum, value_ptr can point to different types (int, uint8_t, etc.)
            // Read as unsigned char first to handle uint8_t, then cast to int
            int enum_value = 0;
            if (setting->value_ptr) {
                // Read the value safely - treat as uint8_t first (most common for enums)
                uint8_t* uint8_ptr = static_cast<uint8_t*>(setting->value_ptr);
                enum_value = static_cast<int>(*uint8_ptr);
                
                // Clamp to valid range for enum display
                if (enum_value < 0) enum_value = 0;
                if (enum_value >= setting->enum_count) enum_value = setting->enum_count - 1;
            }
            
            const char* option = "?";
            if (setting->enum_options && enum_value >= 0 && enum_value < setting->enum_count) {
                option = setting->enum_options[enum_value];
                if (!option) option = "?";  // Handle nullptr in enum_options
            }
            snprintf(buffer, sizeof(buffer), "%s%s: %s", prefix, setting->name, option);
            value_str = buffer;
            break;
        }
        
        case SettingType::STRING: {
            const char* str_val = static_cast<const char*>(setting->value_ptr);
            snprintf(buffer, sizeof(buffer), "%s%s: %s", prefix, setting->name, str_val);
            value_str = buffer;
            break;
        }
    }
    
    disp->SetCursor(0, y_pos);
    disp->WriteString(value_str, Font_6x8, true);
}

} // namespace OpenChord
