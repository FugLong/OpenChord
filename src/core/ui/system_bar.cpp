#include "system_bar.h"
#include <cstdio>
#include <cstring>

namespace OpenChord {

SystemBar::SystemBar()
    : display_(nullptr)
    , io_manager_(nullptr)
    , current_track_(nullptr)
    , context_text_(nullptr)
    , battery_percentage_(100.0f)
    , battery_charging_(false)
    , last_battery_update_(0)
{
    track_name_override_[0] = '\0';
}

SystemBar::~SystemBar() {
}

void SystemBar::Init(DisplayManager* display, IOManager* io_manager) {
    display_ = display;
    io_manager_ = io_manager;
    current_track_ = nullptr;
    context_text_ = nullptr;
    track_name_override_[0] = '\0';
    battery_percentage_ = 100.0f;
    battery_charging_ = false;
    last_battery_update_ = 0;
}

void SystemBar::SetContext(const char* context) {
    // Store context text pointer (caller must ensure string remains valid)
    // nullptr means show track name, non-null means show context text
    context_text_ = context;
}

void SystemBar::SetTrack(Track* track) {
    current_track_ = track;
}

void SystemBar::SetTrackName(const char* name) {
    if (name) {
        strncpy(track_name_override_, name, sizeof(track_name_override_) - 1);
        track_name_override_[sizeof(track_name_override_) - 1] = '\0';
    } else {
        track_name_override_[0] = '\0';
    }
}

void SystemBar::Update() {
    if (!IsHealthy()) return;
    
    // Update battery percentage periodically
    UpdateBattery();
}

void SystemBar::Render() {
    if (!IsHealthy()) return;
    
    auto* disp = display_->GetDisplay();
    if (!disp) return;
    
    // System bar is top 8 pixels (1 line)
    // Layout: Track name (left) | Battery (right)
    
    // Render track name (left side)
    RenderTrackName();
    
    // Render battery (right side)
    RenderBattery();
    
    // Draw dividing line at bottom of system bar (at y=7, 1 pixel thick)
    // System bar is pixels 0-7, so line at y=7 is the bottom edge
    for (int x = 0; x < 128; x++) {
        disp->DrawPixel(x, 7, true);
    }
}

void SystemBar::UpdateBattery() {
    if (!io_manager_) return;
    
    // Update battery percentage periodically (every second)
    // This is called from Update(), which runs at 1kHz, so check every 1000 calls
    static uint32_t update_counter = 0;
    if (++update_counter >= BATTERY_UPDATE_INTERVAL_MS) {
        update_counter = 0;
        
        auto* analog_mgr = io_manager_->GetAnalog();
        if (analog_mgr) {
            battery_percentage_ = analog_mgr->GetBatteryPercentage();
            battery_charging_ = analog_mgr->IsBatteryCharging();
        }
    }
}

void SystemBar::RenderTrackName() {
    auto* disp = display_->GetDisplay();
    if (!disp) return;
    
    const char* display_text = nullptr;
    
    // Priority order:
    // 1. Context text (e.g., "Debug Mode") - overrides everything
    // 2. Track name override (if explicitly set)
    // 3. Default track name ("Track 1" or "No Track")
    
    if (context_text_ && context_text_[0] != '\0') {
        display_text = context_text_;
    } else if (track_name_override_[0] != '\0') {
        display_text = track_name_override_;
    } else if (current_track_) {
        // Get track name from track (if Track interface has GetName method)
        // For now, use default "Track 1" format
        display_text = "Track 1";  // TODO: Get actual track name from Track interface
    } else {
        display_text = "No Track";
    }
    
    // Render text on left side
    disp->SetCursor(0, 0);
    disp->WriteString(display_text, Font_6x8, true);
}

void SystemBar::RenderBattery() {
    auto* disp = display_->GetDisplay();
    if (!disp) return;
    
    // Render battery percentage on right side with charging indicator
    char battery_str[12];
    if (battery_charging_) {
        // Show charging indicator: "100%+" (plus sign indicates charging)
        snprintf(battery_str, sizeof(battery_str), "%.0f%%+", battery_percentage_);
    } else {
        snprintf(battery_str, sizeof(battery_str), "%.0f%%", battery_percentage_);
    }
    
    // Right-align the battery text
    // Font is 6x8, so each char is 6 pixels wide
    // Screen is 128 pixels wide
    int text_width = strlen(battery_str) * 6;
    int x_pos = 128 - text_width;
    if (x_pos < 0) x_pos = 0;
    
    disp->SetCursor(x_pos, 0);
    disp->WriteString(battery_str, Font_6x8, true);
}

} // namespace OpenChord
