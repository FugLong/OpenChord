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
    , render_interval_ms_(100)  // 10 FPS
    , last_render_time_(0)
    , needs_refresh_(true)
{
}

MainUI::~MainUI() {
}

void MainUI::Init(DisplayManager* display, InputManager* input_manager) {
    display_ = display;
    input_manager_ = input_manager;
    track_ = nullptr;
    last_render_time_ = 0;
    needs_refresh_ = true;
}

void MainUI::SetTrack(Track* track) {
    track_ = track;
    needs_refresh_ = true;
}

void MainUI::SetChordPlugin(ChordMappingInput* chord_plugin) {
    chord_plugin_ = chord_plugin;
    needs_refresh_ = true;
}

void MainUI::Update() {
    if (!display_ || !display_->IsHealthy()) return;
    
    // Update render timer
    if (last_render_time_ >= render_interval_ms_ || needs_refresh_) {
        RenderDefaultView();
        last_render_time_ = 0;
        needs_refresh_ = false;
    } else {
        last_render_time_++;
    }
}

void MainUI::Refresh() {
    needs_refresh_ = true;
}

void MainUI::RenderDefaultView() {
    if (!display_ || !display_->IsHealthy()) return;
    
    daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>* disp = display_->GetDisplay();
    if (!disp) return;
    
    // Clear display
    disp->Fill(false);
    
    // Render chord name (primary information)
    RenderChordName();
    
    // Update display
    disp->Update();
}

void MainUI::RenderChordName() {
    if (!display_ || !display_->IsHealthy()) return;
    
    daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>* disp = display_->GetDisplay();
    if (!disp) return;
    
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
    
    // Display layout:
    // Line 0: Track name or key info
    // Line 24: Chord name (large, centered)
    // Line 48: Preset info
    
    int y = 0;
    
    // Top line: Key info
    if (key_text[0] != '\0') {
        disp->SetCursor(0, y);
        disp->WriteString(key_text, Font_6x8, true);
        y += 10;
    }
    
    // Center: Chord name (large font)
    disp->SetCursor(0, 24);
    disp->WriteString(chord_text, Font_11x18, true);
    
    // Bottom: Preset info
    if (preset_text[0] != '\0') {
        disp->SetCursor(0, 54);
        disp->WriteString(preset_text, Font_6x8, true);
    }
}

} // namespace OpenChord

