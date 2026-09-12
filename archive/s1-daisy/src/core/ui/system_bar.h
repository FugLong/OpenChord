#pragma once

#include "../io/display_manager.h"
#include "../io/io_manager.h"
#include "../tracks/track_interface.h"
#include <cstdint>

namespace OpenChord {

/**
 * System Bar - Always-visible top bar showing system information
 * 
 * Displays:
 * - Left: Battery percentage (e.g., "85%")
 * - Center: Current track name/number (e.g., "Track 1")
 * - Right: Current context/mode indicator (e.g., "Menu", "Input", "DEBUG")
 */
class SystemBar {
public:
    SystemBar();
    ~SystemBar();
    
    // Initialization
    void Init(DisplayManager* display, IOManager* io_manager);
    
    // Update and render
    void Update();
    void Render();
    
    // Set current context (overrides track name display, e.g., "Debug Mode", "Menu")
    void SetContext(const char* context);  // Set to nullptr to show track name again
    
    // Set current track (for center of bar)
    void SetTrack(Track* track);
    void SetTrackName(const char* name);  // Override track name display
    
    // Health check
    bool IsHealthy() const { return display_ != nullptr && display_->IsHealthy(); }
    
private:
    DisplayManager* display_;
    IOManager* io_manager_;
    Track* current_track_;
    const char* context_text_;  // Current context text (nullptr = normal mode)
    char track_name_override_[16];  // Override track name (empty = use track name)
    
    // Cached values (updated periodically)
    float battery_percentage_;
    bool battery_charging_;
    uint32_t last_battery_update_;  // 0 = never updated, >0 = last update time/counter
    static constexpr uint32_t BATTERY_UPDATE_INTERVAL_MS = 1000;  // Update battery display every second
    
    void RenderBattery();
    void RenderTrackName();
    void RenderContext();
    void UpdateBattery();
};

} // namespace OpenChord
