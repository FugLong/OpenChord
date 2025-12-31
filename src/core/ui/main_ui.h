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
    
    // Update - call periodically to update state (not rendering)
    void Update();
    
    // Render - renders content area (does NOT clear display or call Update())
    // This is called by UIManager, which handles display lifecycle
    void Render(DisplayManager* display);
    
    // Set track for chord display (optional)
    void SetTrack(Track* track);
    
    // Set chord plugin directly (for direct access without RTTI)
    void SetChordPlugin(class ChordMappingInput* chord_plugin);
    
    // Set chromatic plugin directly (for displaying active notes)
    void SetChromaticPlugin(class ChromaticInput* plugin);
    
    // Health check
    bool IsHealthy() const { return display_ != nullptr; }
    
private:
    DisplayManager* display_;  // Kept for compatibility, but UIManager owns display lifecycle
    InputManager* input_manager_;
    Track* track_;
    class ChordMappingInput* chord_plugin_;  // Direct reference to chord plugin
    class ChromaticInput* chromatic_plugin_;  // Direct reference to chromatic plugin
    
    // Render methods
    void RenderChordName(DisplayManager* display);
    void RenderChromaticNotes(DisplayManager* display);
};

} // namespace OpenChord

