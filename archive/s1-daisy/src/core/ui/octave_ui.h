#pragma once

#include "../io/display_manager.h"
#include "../midi/octave_shift.h"
#include <cstdint>

namespace OpenChord {

/**
 * Octave Shift UI - Temporary overlay for adjusting octave shift
 * 
 * Activated by joystick button click in normal play mode.
 * Shows current octave shift and allows adjustment with left/right joystick.
 */
class OctaveUI {
public:
    OctaveUI();
    ~OctaveUI();
    
    // Initialization
    void Init(DisplayManager* display, OctaveShift* octave_shift);
    
    // State management
    bool IsActive() const { return is_active_; }
    void Activate();  // Show UI
    void Deactivate(); // Hide UI
    
    // Update (handles joystick input for adjustment)
    // Pass current time in milliseconds for timeout handling
    void Update(float joystick_x, uint32_t current_time_ms);
    
    // Render
    void Render();
    
    // Health check
    bool IsHealthy() const { return display_ != nullptr && octave_shift_ != nullptr; }
    
private:
    DisplayManager* display_;
    OctaveShift* octave_shift_;
    bool is_active_;
    uint32_t auto_hide_timeout_;
    static constexpr uint32_t AUTO_HIDE_TIMEOUT_MS = 2000;  // Hide after 2 seconds of no input
    uint32_t last_adjust_time_;
    
    void RenderOctaveDisplay();
};

} // namespace OpenChord

