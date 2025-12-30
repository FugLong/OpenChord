#include "main_ui.h"
#include "../music/chord_engine.h"
#include "../../plugins/input/chord_mapping_input.h"
#include <cstring>
#include <cstdio>

namespace OpenChord {

MainUI::MainUI()
    : display_(nullptr)
    , input_manager_(nullptr)
    , track_(nullptr)
    , chord_plugin_(nullptr)
{
}

MainUI::~MainUI() {
}

void MainUI::Init(DisplayManager* display, InputManager* input_manager) {
    display_ = display;
    input_manager_ = input_manager;
    track_ = nullptr;
}

void MainUI::SetTrack(Track* track) {
    track_ = track;
}

void MainUI::SetChordPlugin(ChordMappingInput* chord_plugin) {
    chord_plugin_ = chord_plugin;
}

void MainUI::Update() {
    // MainUI no longer manages its own rendering
    // State updates happen here if needed in the future
}

void MainUI::Render(DisplayManager* display) {
    if (!display) return;
    
    RenderChordName(display);
}

void MainUI::RenderChordName(DisplayManager* display) {
    if (!display || !display->IsHealthy()) return;
    
    daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>* disp = display->GetDisplay();
    if (!disp) return;
    
    // Content area starts at y=10 (below system bar with spacing)
    
    const char* chord_text = "No Chord";
    char key_text[16] = "";
    char preset_text[16] = "";
    
    // Get chord and key/preset info from chord plugin if available
    if (chord_plugin_) {
        const Chord* chord = chord_plugin_->GetCurrentChord();
        if (chord && chord->note_count > 0) {
            chord_text = chord->name;
        }
        
        // Get key name
        MusicalKey key = chord_plugin_->GetCurrentKey();
        const char* note_names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        const char* mode_names[] = {"Maj", "Dor", "Phr", "Lyd", "Mix", "Min", "Loc"};
        
        int note_idx = key.root_note % 12;
        int mode_idx = static_cast<int>(key.mode);
        
        if (note_idx >= 0 && note_idx < 12 && mode_idx >= 0 && mode_idx < 7) {
            snprintf(key_text, sizeof(key_text), "%s %s", note_names[note_idx], mode_names[mode_idx]);
        }
        
        // Get preset name (using a temporary ChordEngine instance for lookup)
        int preset_idx = chord_plugin_->GetCurrentJoystickPreset();
        ChordEngine engine;
        const JoystickPreset* preset = engine.GetJoystickPreset(preset_idx);
        if (preset && preset->name) {
            snprintf(preset_text, sizeof(preset_text), "Preset: %s", preset->name);
        }
    }
    
    // Content area layout (offset by 10 pixels: 8px system bar + 1px line + 1px spacing):
    // Content area starts at y=10, screen is 64px tall, so content area is y=10 to y=63
    // y=10: Key info
    // y=26: Chord name (large, centered in available space)
    // y=55: Preset info (6x8 font, so 55+8=63, fits at bottom)
    
    int y = 10;  // Start below system bar with spacing
    
    // Top line: Key info
    if (key_text[0] != '\0') {
        disp->SetCursor(0, y);
        disp->WriteString(key_text, Font_6x8, true);
        y += 10;
    }
    
    // Center: Chord name (large font, centered vertically in content area)
    // Content area is y=10 to y=63, center is around y=26 (accounting for 18px font height)
    disp->SetCursor(0, 26);
    disp->WriteString(chord_text, Font_11x18, true);
    
    // Bottom: Preset info (positioned to fit within screen bounds)
    if (preset_text[0] != '\0') {
        disp->SetCursor(0, 55);
        disp->WriteString(preset_text, Font_6x8, true);
    }
}

} // namespace OpenChord

