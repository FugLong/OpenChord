#include "chord_mapping_input.h"
#include "../../core/io/input_manager.h"
#include "../../core/tracks/track_interface.h"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace OpenChord {

ChordMappingInput::ChordMappingInput()
    : input_manager_(nullptr)
    , track_(nullptr)
    , octave_ui_check_func_(nullptr)
    , active_(true)
    , initialized_(false)
    , chord_active_(false)
    , current_joystick_preset_index_(0)
    , current_joystick_preset_(nullptr)
    , joystick_x_(0.0f)
    , joystick_y_(0.0f)
    , current_joystick_direction_(JoystickDirection::CENTER)
    , prev_joystick_direction_(JoystickDirection::CENTER)
    , pending_read_pos_(0)
    , pending_write_pos_(0)
{
    std::memset(prev_button_states_, false, sizeof(prev_button_states_));
    pending_events_.resize(128);  // Buffer for MIDI events
    current_chord_.note_count = 0;
    
    // Initialize settings
    InitializeSettings();
}

ChordMappingInput::~ChordMappingInput() {
}

void ChordMappingInput::Init() {
    if (!input_manager_) return;
    
    active_ = false;  // Start inactive - Piano is the default
    initialized_ = true;
    
    // Set default key in track (C Major / Ionian) if track is available
    if (track_) {
        SetKey(MusicalKey(0, MusicalMode::IONIAN));  // C Ionian (C Major)
    }
    SetJoystickPreset(0);
    
    // Reset state
    chord_active_ = false;
    current_chord_.note_count = 0;
    joystick_x_ = 0.0f;
    joystick_y_ = 0.0f;
    current_joystick_direction_ = JoystickDirection::CENTER;
    prev_joystick_direction_ = JoystickDirection::CENTER;
    std::memset(prev_button_states_, false, sizeof(prev_button_states_));
    
    pending_read_pos_ = 0;
    pending_write_pos_ = 0;
}

void ChordMappingInput::Process(float* in, float* out, size_t size) {
    // This plugin doesn't process audio directly
    // It only generates MIDI events
}

void ChordMappingInput::Update() {
    if (!initialized_ || !active_ || !input_manager_) return;
    
    // Process button state changes
    ProcessButtons();
    
    // Process joystick to update chord variations
    ProcessJoystick();
}

void ChordMappingInput::UpdateUI() {
    // UI updates handled by main UI system
}

void ChordMappingInput::HandleEncoder(int encoder, float delta) {
    // Could be used for key selection or preset cycling
    // For now, no encoder handling
}

void ChordMappingInput::HandleButton(int button, bool pressed) {
    // Buttons are processed via ProcessButtons() which polls state directly
    // This method could be used for system buttons if needed
}

void ChordMappingInput::HandleJoystick(float x, float y) {
    // Joystick is processed via ProcessJoystick() which polls state directly
    // This method receives joystick values from Track system
    joystick_x_ = x;
    joystick_y_ = y;
}

void ChordMappingInput::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    // Save state: active flag and preset index
    // Note: key is now stored in track, not in plugin state
    struct State {
        bool active;
        int preset_index;
    };
    
    State state;
    state.active = active_;
    state.preset_index = current_joystick_preset_index_;
    
    std::memcpy(buffer, &state, sizeof(State));
    *size = sizeof(State);
}

void ChordMappingInput::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(bool)) return;
    
    struct State {
        bool active;
        int preset_index;
    };
    
    struct LegacyState {
        bool active;
        MusicalKey key;
        int preset_index;
    };
    
    if (size >= sizeof(State)) {
        const State* state = reinterpret_cast<const State*>(buffer);
        active_ = state->active;
        SetJoystickPreset(state->preset_index);
    } else if (size >= sizeof(LegacyState)) {
        // Legacy format: load old state but key is now in track
        const LegacyState* state = reinterpret_cast<const LegacyState*>(buffer);
        active_ = state->active;
        SetJoystickPreset(state->preset_index);
        // Key is now stored in track, not here
    } else {
        // Very old format: just active flag
        active_ = *reinterpret_cast<const bool*>(buffer);
        SetJoystickPreset(0);
    }
}

size_t ChordMappingInput::GetStateSize() const {
    return sizeof(bool) + sizeof(int);  // active, preset_index (key removed - now track-level)
}

void ChordMappingInput::GenerateMIDI(MidiEvent* events, size_t* count, size_t max_events) {
    *count = 0;
    
    if (!active_ || !initialized_) return;
    
    // Read from pending events buffer
    while (*count < max_events && pending_read_pos_ != pending_write_pos_) {
        events[*count] = pending_events_[pending_read_pos_];
        pending_read_pos_ = (pending_read_pos_ + 1) % pending_events_.size();
        (*count)++;
    }
}

void ChordMappingInput::ProcessMIDI(const MidiEvent* events, size_t count) {
    // This plugin generates MIDI, doesn't process incoming MIDI
    // Could be enhanced to transform incoming MIDI if needed
}

void ChordMappingInput::ProcessButtons() {
    if (!input_manager_) return;
    
    ButtonInputHandler& buttons = input_manager_->GetButtons();
    
    // Track which buttons are currently pressed
    bool currently_pressed[7] = {false};
    int pressed_count = 0;
    int first_pressed = -1;
    
    // First pass: determine current button states
    for (int i = 0; i < 7; i++) {
        MusicalButton btn = ButtonIndexToMusicalButton(i);
        bool current_pressed = buttons.IsMusicalButtonPressed(btn);
        currently_pressed[i] = current_pressed;
        
        if (current_pressed) {
            pressed_count++;
            if (first_pressed < 0) {
                first_pressed = i;
            }
        }
    }
    
    // Second pass: handle button state changes
    for (int i = 0; i < 7; i++) {
        bool current_pressed = currently_pressed[i];
        bool prev_pressed = prev_button_states_[i];
        
        // Detect button press
        if (!prev_pressed && current_pressed) {
            // Button just pressed
            // Send NOTE_OFF for all currently active notes (if switching buttons)
            if (chord_active_ && current_chord_.note_count > 0) {
                for (uint8_t n = 0; n < current_chord_.note_count; n++) {
                    MidiEvent event;
                    event.type = static_cast<uint8_t>(MidiEvent::Type::NOTE_OFF);
                    event.channel = 0;
                    event.data1 = current_chord_.notes[n];
                    event.data2 = 0;
                    event.timestamp = 0;
                    
                    size_t next_write = (pending_write_pos_ + 1) % pending_events_.size();
                    if (next_write != pending_read_pos_) {
                        pending_events_[pending_write_pos_] = event;
                        pending_write_pos_ = next_write;
                    }
                }
            }
            
            // Generate new chord for the pressed button
            UpdateChord(i);
            chord_active_ = true;
            
            // Generate NOTE_ON events for all notes in new chord
            if (current_chord_.note_count > 0) {
                for (uint8_t n = 0; n < current_chord_.note_count; n++) {
                    MidiEvent event;
                    event.type = static_cast<uint8_t>(MidiEvent::Type::NOTE_ON);
                    event.channel = 0;
                    event.data1 = current_chord_.notes[n];
                    event.data2 = 100;  // Velocity
                    event.timestamp = 0;
                    
                    size_t next_write = (pending_write_pos_ + 1) % pending_events_.size();
                    if (next_write != pending_read_pos_) {
                        pending_events_[pending_write_pos_] = event;
                        pending_write_pos_ = next_write;
                    }
                }
            }
        }
        
        // Detect button release
        if (prev_pressed && !current_pressed) {
            // Button just released
            // If this was the active button, send NOTE_OFF for all notes
            if (chord_active_ && current_chord_.note_count > 0) {
                for (uint8_t n = 0; n < current_chord_.note_count; n++) {
                    MidiEvent event;
                    event.type = static_cast<uint8_t>(MidiEvent::Type::NOTE_OFF);
                    event.channel = 0;
                    event.data1 = current_chord_.notes[n];
                    event.data2 = 0;
                    event.timestamp = 0;
                    
                    size_t next_write = (pending_write_pos_ + 1) % pending_events_.size();
                    if (next_write != pending_read_pos_) {
                        pending_events_[pending_write_pos_] = event;
                        pending_write_pos_ = next_write;
                    }
                }
            }
            
            // Check if any buttons are still pressed
            bool any_pressed = false;
            int new_first_pressed = -1;
            for (int j = 0; j < 7; j++) {
                if (currently_pressed[j]) {
                    any_pressed = true;
                    if (new_first_pressed < 0) {
                        new_first_pressed = j;
                    }
                }
            }
            
            if (any_pressed && new_first_pressed >= 0) {
                // Another button is still pressed - switch to that button's chord
                UpdateChord(new_first_pressed);
                chord_active_ = true;
                
                // Generate NOTE_ON events for the new chord
                if (current_chord_.note_count > 0) {
                    for (uint8_t n = 0; n < current_chord_.note_count; n++) {
                        MidiEvent event;
                        event.type = static_cast<uint8_t>(MidiEvent::Type::NOTE_ON);
                        event.channel = 0;
                        event.data1 = current_chord_.notes[n];
                        event.data2 = 100;
                        event.timestamp = 0;
                        
                        size_t next_write = (pending_write_pos_ + 1) % pending_events_.size();
                        if (next_write != pending_read_pos_) {
                            pending_events_[pending_write_pos_] = event;
                            pending_write_pos_ = next_write;
                        }
                    }
                }
            } else {
                // No buttons pressed - clear chord
                chord_active_ = false;
                current_chord_.note_count = 0;
            }
        }
        
        prev_button_states_[i] = current_pressed;
    }
}

void ChordMappingInput::ProcessJoystick() {
    if (!input_manager_) return;
    
    // Don't process joystick if octave UI is active (joystick is used for octave adjustment)
    if (octave_ui_check_func_ && octave_ui_check_func_()) {
        // Reset to center position when octave UI takes over
        current_joystick_direction_ = JoystickDirection::CENTER;
        prev_joystick_direction_ = JoystickDirection::CENTER;
        return;
    }
    
    // Get joystick position
    JoystickInputHandler& joystick = input_manager_->GetJoystick();
    float x, y;
    joystick.GetPosition(&x, &y);
    
    // Store for later use
    joystick_x_ = x;
    joystick_y_ = y;
    
    // Determine joystick direction
    JoystickDirection new_direction = GetJoystickDirectionFromXY(x, y);
    
    // Update current direction immediately so UpdateChord can use it
    JoystickDirection old_direction = current_joystick_direction_;
    current_joystick_direction_ = new_direction;
    
    // Check if direction changed (including to/from CENTER)
    bool direction_changed = (new_direction != prev_joystick_direction_);
    
    // If direction changed AND a button is pressed, update chord
    if (direction_changed) {
        bool any_button_pressed = false;
        int pressed_button_index = -1;
        
        for (int i = 0; i < 7; i++) {
            MusicalButton btn = ButtonIndexToMusicalButton(i);
            if (input_manager_->GetButtons().IsMusicalButtonPressed(btn)) {
                any_button_pressed = true;
                pressed_button_index = i;
                break;
            }
        }
        
        if (any_button_pressed && pressed_button_index >= 0) {
            // Turn off old chord notes
            if (current_chord_.note_count > 0) {
                for (uint8_t n = 0; n < current_chord_.note_count; n++) {
                    MidiEvent event;
                    event.type = static_cast<uint8_t>(MidiEvent::Type::NOTE_OFF);
                    event.channel = 0;
                    event.data1 = current_chord_.notes[n];
                    event.data2 = 0;
                    event.timestamp = 0;
                    
                    size_t next_write = (pending_write_pos_ + 1) % pending_events_.size();
                    if (next_write != pending_read_pos_) {
                        pending_events_[pending_write_pos_] = event;
                        pending_write_pos_ = next_write;
                    }
                }
            }
            
            // Generate new chord with joystick variation (or base chord if CENTER)
            // current_joystick_direction_ is already updated above, so UpdateChord will use the correct direction
            UpdateChord(pressed_button_index);
            
            // Turn on new chord notes
            if (current_chord_.note_count > 0) {
                for (uint8_t n = 0; n < current_chord_.note_count; n++) {
                    MidiEvent event;
                    event.type = static_cast<uint8_t>(MidiEvent::Type::NOTE_ON);
                    event.channel = 0;
                    event.data1 = current_chord_.notes[n];
                    event.data2 = 100;
                    event.timestamp = 0;
                    
                    size_t next_write = (pending_write_pos_ + 1) % pending_events_.size();
                    if (next_write != pending_read_pos_) {
                        pending_events_[pending_write_pos_] = event;
                        pending_write_pos_ = next_write;
                    }
                }
            }
        }
    }
    
    // Update previous direction for next iteration
    prev_joystick_direction_ = new_direction;
}

void ChordMappingInput::UpdateChord(int button_index) {
    if (button_index < 0 || button_index >= 7) return;
    
    // Physical button index needs to be converted to scale degree
    // Physical order (left-to-right): 0, 4, 1, 5, 2, 6, 3 → Scale degrees: I, II, III, IV, V, VI, VII
    int scale_degree = chord_engine_.PhysicalButtonToScaleDegree(button_index);
    
    // Get MIDI note for this scale degree in the current key/mode
    MusicalKey key = GetCurrentKey();
    uint8_t root_midi_note;
    chord_engine_.GetButtonMapping(key, button_index, &root_midi_note);
    
    // Get base chord quality for this scale degree in the current mode
    ChordQuality base_quality = chord_engine_.GetChordQualityForDegree(key.mode, scale_degree);
    
    // Apply joystick variation if joystick is not centered
    ChordQuality final_quality = base_quality;
    if (current_joystick_direction_ != JoystickDirection::CENTER) {
        final_quality = chord_engine_.ApplyJoystickVariation(
            base_quality, 
            static_cast<int>(current_joystick_direction_), 
            current_joystick_preset_index_
        );
    }
    
    // Generate chord (always root position for now, could add inversion later)
    chord_engine_.GenerateChord(
        &current_chord_, 
        root_midi_note, 
        final_quality, 
        ChordInversion::ROOT
    );
    
    // Note: Octave shift is applied in main.cpp to all MIDI events before sending
}

JoystickDirection ChordMappingInput::GetJoystickDirectionFromXY(float x, float y) const {
    // User says joystick output is -0.5 to 0.5 range (normalized -1.0 to 1.0)
    // Left and down are negative, right and up are positive
    // Cardinal directions: >= 0.48 or <= -0.48
    // Diagonals: both axes >= 0.24 or <= -0.24
    const float cardinal_threshold = 0.48f;    // For UP, DOWN, LEFT, RIGHT
    const float diagonal_threshold = 0.24f;    // For diagonals (both axes need to meet this)
    const float dead_zone = 0.1f;              // Below this = CENTER
    
    float abs_x = std::abs(x);
    float abs_y = std::abs(y);
    float distance = std::sqrt(x * x + y * y);
    
    // Dead zone check first
    if (distance < dead_zone) {
        return JoystickDirection::CENTER;
    }
    
    // Determine if this is more diagonal or cardinal by comparing axis magnitudes
    // If both axes are significant (within ~30% of each other), treat as diagonal
    float axis_ratio = (abs_x > abs_y) ? (abs_y / abs_x) : (abs_x / abs_y);
    bool is_diagonal = (axis_ratio > 0.6f) && (abs_x >= diagonal_threshold) && (abs_y >= diagonal_threshold);
    
    // Check for diagonal directions if both axes are significant
    if (is_diagonal) {
        // Both axes meet diagonal threshold and are balanced - determine which diagonal
        // x > 0 = right, y > 0 = up, x < 0 = left, y < 0 = down
        if (x > 0 && y > 0) return JoystickDirection::UP_RIGHT;      // Northeast (right + up)
        if (x > 0 && y < 0) return JoystickDirection::DOWN_RIGHT;    // Southeast (right + down)
        if (x < 0 && y > 0) return JoystickDirection::UP_LEFT;       // Northwest (left + up)
        if (x < 0 && y < 0) return JoystickDirection::DOWN_LEFT;     // Southwest (left + down)
    }
    
    // Check cardinal directions (one axis dominates and meets threshold)
    // Vertical dominates
    if (abs_y >= cardinal_threshold && abs_y >= abs_x) {
        if (y > 0) return JoystickDirection::UP;      // Up (positive Y)
        else return JoystickDirection::DOWN;          // Down (negative Y)
    }
    
    // Horizontal dominates
    if (abs_x >= cardinal_threshold && abs_x >= abs_y) {
        if (x > 0) return JoystickDirection::RIGHT;   // Right (positive X)
        else return JoystickDirection::LEFT;          // Left (negative X)
    }
    
    // If we get here, below thresholds but not in dead zone - return CENTER
    return JoystickDirection::CENTER;
}

MusicalButton ChordMappingInput::ButtonIndexToMusicalButton(int index) const {
    if (index < 0 || index >= 7) {
        return MusicalButton::WHITE_0;
    }
    
    static const MusicalButton mapping[] = {
        MusicalButton::WHITE_0,  // 0 = F
        MusicalButton::WHITE_1,  // 1 = G
        MusicalButton::WHITE_2,  // 2 = A
        MusicalButton::WHITE_3,  // 3 = B
        MusicalButton::BLACK_0,  // 4 = F#
        MusicalButton::BLACK_1,  // 5 = G#
        MusicalButton::BLACK_2   // 6 = A#
    };
    
    return mapping[index];
}

void ChordMappingInput::SetKey(MusicalKey key) {
    if (track_) {
        track_->SetKey(key);
    }
    
    // If chord is active, regenerate it with new key
    if (chord_active_) {
        for (int i = 0; i < 7; i++) {
            if (prev_button_states_[i]) {
                UpdateChord(i);
                break;
            }
        }
    }
}

MusicalKey ChordMappingInput::GetCurrentKey() const {
    if (track_) {
        return track_->GetKey();
    }
    // Fallback to default if no track
    return MusicalKey(0, MusicalMode::IONIAN);
}

void ChordMappingInput::SetJoystickPreset(int preset_index) {
    current_joystick_preset_index_ = preset_index;
    current_joystick_preset_ = chord_engine_.GetJoystickPreset(preset_index);
    
    if (!current_joystick_preset_) {
        // Fallback to preset 0 if invalid
        current_joystick_preset_index_ = 0;
        current_joystick_preset_ = chord_engine_.GetJoystickPreset(0);
    }
    
    // If chord is active, regenerate it with new preset
    if (chord_active_ && current_joystick_direction_ != JoystickDirection::CENTER) {
        for (int i = 0; i < 7; i++) {
            if (prev_button_states_[i]) {
                UpdateChord(i);
                break;
            }
        }
    }
}

// Settings implementation
void ChordMappingInput::InitializeSettings() {
    // Setting 0: Joystick Preset (0 to preset_count-1, enum)
    // Note: Key and Mode settings removed - now track-level settings
    settings_[0].name = "Joystick Preset";
    settings_[0].type = SettingType::INT;
    settings_[0].value_ptr = &current_joystick_preset_index_;
    settings_[0].min_value = 0.0f;
    settings_[0].max_value = static_cast<float>(chord_engine_.GetJoystickPresetCount() - 1);
    settings_[0].step_size = 1.0f;
    settings_[0].enum_options = nullptr;
    settings_[0].enum_count = 0;
    settings_[0].on_change_callback = nullptr;
}

int ChordMappingInput::GetSettingCount() const {
    return SETTING_COUNT;
}

const PluginSetting* ChordMappingInput::GetSetting(int index) const {
    if (index < 0 || index >= SETTING_COUNT) {
        return nullptr;
    }
    
    // Update enum count for preset (in case it changed)
    if (index == 0) {
        settings_[0].max_value = static_cast<float>(chord_engine_.GetJoystickPresetCount() - 1);
    }
    
    return &settings_[index];
}

void ChordMappingInput::OnSettingChanged(int setting_index) {
    switch (setting_index) {
        case 0:  // Joystick Preset changed
            // Preset index changed, update preset
            SetJoystickPreset(current_joystick_preset_index_);
            break;
    }
}

} // namespace OpenChord
