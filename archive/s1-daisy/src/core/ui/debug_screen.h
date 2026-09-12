#pragma once

#include "daisy_seed.h"
#include "debug_view.h"
#include "../io/display_manager.h"
#include "../io/input_manager.h"
#include <cstdint>
#include <vector>

namespace OpenChord {

/**
 * Debug Screen - Simple, flexible debug display system
 * 
 * Features:
 * - Easy registration: Just add a name and render function
 * - Button-based navigation: INPUT button + encoder/joystick
 * - Automatic rendering at configured interval
 * - Zero overhead when disabled
 * 
 * Usage:
 *   // Register a simple debug view (just a name and render function)
 *   debug_screen.AddView("MyDebug", my_render_function);
 *   
 *   // In your render function (called automatically):
 *   void my_render_function(DisplayManager* display) {
 *       auto* disp = display->GetDisplay();
 *       disp->Fill(false);  // Clear (done automatically, but safe to do again)
 *       disp->SetCursor(0, 0);
 *       disp->WriteString("Custom Info", Font_6x8, true);
 *       // ... render whatever you want ...
 *       disp->Update();  // Required!
 *   }
 * 
 * Navigation:
 *   - INPUT button (first button, top row) = Previous view (left/up)
 *   - RECORD button (last button, top row) = Next view (right/down)
 */
class DebugScreen {
public:
    DebugScreen();
    ~DebugScreen();
    
    // Initialization
    void Init(DisplayManager* display, InputManager* input_manager);
    
    // Main update loop (call from main loop)
    void Update();
    
    // Register a debug view (just a name and render function)
    void AddView(const char* name, DebugRenderFunc render_func);
    
    // Navigation
    void NextView();
    void PreviousView();
    void SetView(int index);
    int GetCurrentViewIndex() const { return current_view_index_; }
    int GetViewCount() const { return static_cast<int>(views_.size()); }
    const char* GetCurrentViewName() const;
    
    // Enable/disable
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }
    
    // Configuration
    void SetRenderInterval(uint32_t ms) { render_interval_ms_ = ms; }
    
    // Render - renders content area (does NOT clear display or call Update())
    // This is called by UIManager, which handles display lifecycle
    void Render(DisplayManager* display);
    
    // Health check
    bool IsHealthy() const { return display_ != nullptr && display_->IsHealthy(); }
    
private:
    DisplayManager* display_;
    InputManager* input_manager_;
    
    std::vector<DebugView> views_;
    int current_view_index_;
    bool enabled_;
    
    // Rendering control (interval in milliseconds, assuming 1kHz Update() calls)
    uint32_t render_interval_ms_;
    uint32_t last_render_time_;
    
    // Button combo detection for toggle
    bool prev_input_pressed_;
    bool prev_record_pressed_;
    uint32_t combo_hold_time_;
    bool already_toggled_this_cycle_;  // Prevent multiple toggles per press cycle
    static constexpr uint32_t COMBO_HOLD_THRESHOLD_MS = 500;  // 500ms (0.5 seconds) hold to toggle
    
    void RenderCurrentView();
    void RenderCurrentViewInternal(DisplayManager* display);  // Internal helper that takes display param
    void HandleNavigation();
    void HandleToggleCombo();
    bool IsNavigationButtonPressed() const;
};

} // namespace OpenChord