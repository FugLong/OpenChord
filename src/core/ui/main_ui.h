#pragma once

#include "../io/display_manager.h"
#include "../io/input_manager.h"
#include "../tracks/track_interface.h"
#include "daisy_seed.h"
#include "dev/oled_ssd130x.h"

namespace OpenChord {

/**
 * Main UI - Default user interface for OpenChord
 * 
 * Displays chord name and other primary information
 */
class MainUI {
public:
    MainUI();
    ~MainUI();
    
    // Initialization
    void Init(DisplayManager* display, InputManager* input_manager);
    
    // Update - call from main loop
    void Update();
    
    // Set track for chord display (optional)
    void SetTrack(Track* track);
    
    // Set chord plugin directly (for direct access without RTTI)
    void SetChordPlugin(class ChordMappingInput* chord_plugin);
    
    // Force refresh
    void Refresh();
    
    // Health check
    bool IsHealthy() const { return display_ != nullptr && display_->IsHealthy(); }
    
private:
    DisplayManager* display_;
    InputManager* input_manager_;
    Track* track_;
    class ChordMappingInput* chord_plugin_;  // Direct reference to chord plugin
    
    // Rendering control
    uint32_t render_interval_ms_;
    uint32_t last_render_time_;
    bool needs_refresh_;
    
    // Render methods
    void RenderChordName();
    void RenderDefaultView();
};

} // namespace OpenChord

