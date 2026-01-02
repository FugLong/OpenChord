#include "main_ui.h"
#include "content_area.h"
#include "../music/chord_engine.h"
#include "../../plugins/input/chord_mapping_input.h"
#include "../../plugins/input/piano_input.h"
#include <cstring>
#include <cstdio>

namespace OpenChord {

MainUI::MainUI()
    : display_(nullptr)
    , input_manager_(nullptr)
    , track_(nullptr)
    , chord_plugin_(nullptr)
    , piano_plugin_(nullptr)
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

void MainUI::SetPianoPlugin(PianoInput* plugin) {
    piano_plugin_ = plugin;
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
    
    int y = 10;  // Start below system bar with spacing
    
    // Check if chord mapping plugin is active
    bool chord_mode_active = chord_plugin_ && chord_plugin_->IsActive();
    
    if (chord_mode_active) {
        // Chord mode: show chord name, key, and preset
        const char* chord_text = "----";  // Default: show dashes when no chord
        char key_text[16] = "";
        char preset_text[16] = "";
        
        if (chord_plugin_) {
            const Chord* chord = chord_plugin_->GetCurrentChord();
            if (chord && chord->note_count > 0) {
                chord_text = chord->name;
            }
            // else: keep "----" as default
            
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
        
        // Top line: Key info
        if (key_text[0] != '\0') {
            disp->SetCursor(0, y);
            disp->WriteString(key_text, Font_6x8, true);
            y += 10;
        }
        
        // Center: Chord name (large font, centered vertically in content area)
        // Only render if chord_text is not nullptr
        if (chord_text) {
            int content_start_y = y;
            int content_height = 64 - content_start_y - 8;  // Leave space at bottom for preset
            int font_height = 18;  // Font_11x18 height
            int chord_y = content_start_y + (content_height - font_height) / 2;
            
            // Calculate chord text width for centering
            int chord_text_len = strlen(chord_text);
            int chord_text_width = chord_text_len * 11;  // Font_11x18 width per character
            int chord_x = (128 - chord_text_width) / 2;
            if (chord_x < 0) chord_x = 0;
            
            disp->SetCursor(chord_x, chord_y);
            disp->WriteString(chord_text, Font_11x18, true);
        }
        
        // Bottom: Preset info
        if (preset_text[0] != '\0') {
            disp->SetCursor(0, 55);
            disp->WriteString(preset_text, Font_6x8, true);
        }
    } else {
        // Not in chord mode: show active notes from piano input
        if (piano_plugin_ && piano_plugin_->IsActive()) {
            RenderPianoNotes(display);
        } else if (chord_plugin_) {
            // Fallback: show key only if available
            MusicalKey key = chord_plugin_->GetCurrentKey();
            const char* note_names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
            const char* mode_names[] = {"Maj", "Dor", "Phr", "Lyd", "Mix", "Min", "Loc"};
            
            int note_idx = key.root_note % 12;
            int mode_idx = static_cast<int>(key.mode);
            
            if (note_idx >= 0 && note_idx < 12 && mode_idx >= 0 && mode_idx < 7) {
                char key_text[16] = "";
                snprintf(key_text, sizeof(key_text), "%s %s", note_names[note_idx], mode_names[mode_idx]);
                disp->SetCursor(0, y);
                disp->WriteString(key_text, Font_6x8, true);
            }
        }
    }
}

void MainUI::RenderPianoNotes(DisplayManager* display) {
    if (!display || !display->IsHealthy() || !piano_plugin_) return;
    
    auto* disp = display->GetDisplay();
    if (!disp) return;
    
    int y = 10;  // Start below system bar with spacing
    
    // Show key info at top (like Chords mode)
    if (track_ && piano_plugin_) {
        MusicalKey key = piano_plugin_->GetCurrentKey();
        const char* note_names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        const char* mode_names[] = {"Maj", "Dor", "Phr", "Lyd", "Mix", "Min", "Loc"};
        
        int note_idx = key.root_note % 12;
        int mode_idx = static_cast<int>(key.mode);
        
        if (note_idx >= 0 && note_idx < 12 && mode_idx >= 0 && mode_idx < 7) {
            char key_text[16] = "";
            snprintf(key_text, sizeof(key_text), "%s %s", note_names[note_idx], mode_names[mode_idx]);
            disp->SetCursor(0, y);
            disp->WriteString(key_text, Font_6x8, true);
            y += 10;  // Move down for notes display
        }
    }
    
    // Get active notes from piano plugin
    std::vector<uint8_t> active_notes = piano_plugin_->GetActiveNotes();
    
    // y already set above for key display, adjust for notes
    int notes_y = y;  // Start notes display after key
    
    if (active_notes.empty()) {
        // No notes active - show nothing or "--"
        return;  // Just show nothing
    }
    
    // Build note name string
    char note_text[64] = "";
    const char* note_names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    
    for (size_t i = 0; i < active_notes.size() && i < 7; i++) {
        uint8_t midi_note = active_notes[i];
        int note_idx = midi_note % 12;
        int octave = (midi_note / 12) - 1;  // MIDI note 60 = C4
        
        if (i > 0) {
            strcat(note_text, " ");
        }
        
        char note_str[8];
        snprintf(note_str, sizeof(note_str), "%s%d", note_names[note_idx], octave);
        strcat(note_text, note_str);
    }
    
    // Center the text
    int text_width = strlen(note_text) * 11;  // Font_11x18 width
    int text_x = (128 - text_width) / 2;
    if (text_x < 0) text_x = 0;
    
    int content_start_y = notes_y;
    int content_height = 64 - content_start_y - 8;
    int font_height = 18;
    int text_y = content_start_y + (content_height - font_height) / 2;
    
    disp->SetCursor(text_x, text_y);
    disp->WriteString(note_text, Font_11x18, true);
}

} // namespace OpenChord

