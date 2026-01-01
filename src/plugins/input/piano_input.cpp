#include "piano_input.h"
#include "../../core/io/input_manager.h"
#include "../../core/midi/octave_shift.h"
#include "../../core/tracks/track_interface.h"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace OpenChord {

// Mode names for settings
static const char* mode_names[] = {
    "Ionian", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Aeolian", "Locrian", nullptr
};

// Play mode names for settings
static const char* play_mode_names[] = {
    "Chromatic", "Scale", nullptr
};

// Note names for key root setting
static const char* note_names[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B", nullptr
};

PianoInput::PianoInput()
    : input_manager_(nullptr)
    , octave_shift_(nullptr)
    , track_(nullptr)
    , active_(true)
    , initialized_(false)
    , play_mode_(PlayMode::SCALE)
    , current_key_(MusicalKey(0, MusicalMode::IONIAN))  // C Ionian (C Major)
    , pending_read_pos_(0)
    , pending_write_pos_(0)
    , joystick_x_(0.0f)
    , joystick_y_(0.0f)
    , last_pitch_bend_value_(8192)  // Center (no bend)
    , last_mod_wheel_value_(0)      // Mod wheel at 0
    , mode_setting_value_(static_cast<int>(MusicalMode::IONIAN))
    , play_mode_setting_value_(static_cast<int>(PlayMode::SCALE))
    , key_root_setting_value_(0)  // C (will be synced in Init)
{
    std::memset(prev_button_states_, false, sizeof(prev_button_states_));
    std::memset(current_button_states_, false, sizeof(current_button_states_));
    pending_events_.resize(128);
    
    // Initialize settings
    InitializeSettings();
}

PianoInput::~PianoInput() {
}

void PianoInput::Init() {
    if (!input_manager_) return;
    
    active_ = true;
    initialized_ = true;
    std::memset(prev_button_states_, false, sizeof(prev_button_states_));
    pending_read_pos_ = 0;
    pending_write_pos_ = 0;
    joystick_x_ = 0.0f;
    joystick_y_ = 0.0f;
    last_pitch_bend_value_ = 8192;  // Center (no bend)
    last_mod_wheel_value_ = 0;      // Mod wheel at 0
    
    // Set default key (C Major / Ionian)
    SetKey(MusicalKey(0, MusicalMode::IONIAN));
    SetPlayMode(PlayMode::SCALE);
    
    // Sync key root setting value
    key_root_setting_value_ = current_key_.root_note;
}

void PianoInput::Process(float* in, float* out, size_t size) {
    // No audio processing
}

void PianoInput::Update() {
    if (!initialized_ || !input_manager_) return;
    
    // Ensure we're active if no other plugins are active (default behavior)
    EnsureDefaultActivation();
    
    // Only process if we're actually active
    if (!active_) return;
    
    ProcessButtons();
    ProcessJoystick();
}

void PianoInput::UpdateUI() {
    // No UI updates needed
}

void PianoInput::HandleEncoder(int encoder, float delta) {
    // No encoder handling
}

void PianoInput::HandleButton(int button, bool pressed) {
    // Buttons processed via ProcessButtons()
}

void PianoInput::HandleJoystick(float x, float y) {
    // Store joystick position for processing in Update()
    joystick_x_ = x;
    joystick_y_ = y;
}

void PianoInput::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    // Save: active, play_mode, key (root + mode)
    struct State {
        bool active;
        uint8_t play_mode;
        uint8_t key_root;
        uint8_t key_mode;
    };
    
    State state;
    state.active = active_;
    state.play_mode = static_cast<uint8_t>(play_mode_);
    state.key_root = current_key_.root_note;
    state.key_mode = static_cast<uint8_t>(current_key_.mode);
    
    *size = sizeof(State);
    std::memcpy(buffer, &state, sizeof(State));
}

void PianoInput::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(bool)) return;
    
    struct State {
        bool active;
        uint8_t play_mode;
        uint8_t key_root;
        uint8_t key_mode;
    };
    
    if (size >= sizeof(State)) {
        const State* state = reinterpret_cast<const State*>(buffer);
        active_ = state->active;
        play_mode_ = static_cast<PlayMode>(state->play_mode);
        if (state->key_mode < static_cast<uint8_t>(MusicalMode::COUNT)) {
            current_key_ = MusicalKey(state->key_root, static_cast<MusicalMode>(state->key_mode));
        }
    } else {
        // Legacy: just load active state
        active_ = *reinterpret_cast<const bool*>(buffer);
    }
}

size_t PianoInput::GetStateSize() const {
    struct State {
        bool active;
        uint8_t play_mode;
        uint8_t key_root;
        uint8_t key_mode;
    };
    return sizeof(State);
}

bool PianoInput::IsActive() const {
    // Piano input is active if:
    // 1. Our active flag is true
    // 2. We're initialized
    if (!active_ || !initialized_) {
        return false;
    }
    
    return true;
}

void PianoInput::ProcessMIDI(const MidiEvent* events, size_t count) {
    // Piano input doesn't process incoming MIDI
}

void PianoInput::GenerateMIDI(MidiEvent* events, size_t* count, size_t max_events) {
    if (!initialized_ || !active_ || !input_manager_ || !events || !count) {
        *count = 0;
        return;
    }
    
    *count = 0;
    
    // Read pending events from buffer
    while (pending_read_pos_ != pending_write_pos_ && *count < max_events) {
        events[*count] = pending_events_[pending_read_pos_];
        (*count)++;
        
        pending_read_pos_++;
        if (pending_read_pos_ >= pending_events_.size()) {
            pending_read_pos_ = 0;
        }
    }
}

void PianoInput::ProcessButtons() {
    if (!input_manager_) return;
    
    auto& button_handler = input_manager_->GetButtons();
    
    for (int i = 0; i < 7; i++) {
        MusicalButton button = static_cast<MusicalButton>(i);
        bool current_pressed = button_handler.IsMusicalButtonPressed(button);
        bool prev_pressed = prev_button_states_[i];
        
        // Update current state
        current_button_states_[i] = current_pressed;
        
        if (current_pressed != prev_pressed) {
            uint8_t midi_note;
            
            // Get MIDI note based on current play mode
            if (play_mode_ == PlayMode::SCALE) {
                midi_note = GetScaleNote(i);
            } else {
                midi_note = GetMidiNote(i);
            }
            
            // Note: Octave shift is applied in main.cpp to all MIDI events before sending
            
            if (current_pressed && !prev_pressed) {
                // Button pressed: send NOTE_ON
                if (pending_write_pos_ < pending_events_.size()) {
                    MidiEvent& event = pending_events_[pending_write_pos_];
                    event.type = static_cast<uint8_t>(MidiEvent::Type::NOTE_ON);
                    event.channel = 0;
                    event.data1 = midi_note;  // Base note (octave shift applied in main.cpp)
                    event.data2 = 100;  // Default velocity
                    event.timestamp = 0;
                    
                    pending_write_pos_++;
                    if (pending_write_pos_ >= pending_events_.size()) {
                        pending_write_pos_ = 0;
                    }
                }
            } else if (!current_pressed && prev_pressed) {
                // Button released: send NOTE_OFF
                if (pending_write_pos_ < pending_events_.size()) {
                    MidiEvent& event = pending_events_[pending_write_pos_];
                    event.type = static_cast<uint8_t>(MidiEvent::Type::NOTE_OFF);
                    event.channel = 0;
                    event.data1 = midi_note;  // Base note (octave shift applied in main.cpp)
                    event.data2 = 0;
                    event.timestamp = 0;
                    
                    pending_write_pos_++;
                    if (pending_write_pos_ >= pending_events_.size()) {
                        pending_write_pos_ = 0;
                    }
                }
            }
            
            prev_button_states_[i] = current_pressed;
        }
    }
}

void PianoInput::ProcessJoystick() {
    if (!initialized_ || !active_ || !input_manager_) return;
    
    // Process pitch bend (Y axis: up/down)
    // Pitch bend bends the actual MIDI note chromatically (by semitones), not by scale degrees
    // This allows playing in key with buttons, but bending chromatically to go outside the key
    // Apply dead zone to avoid noise when joystick is centered
    const float dead_zone = 0.05f;
    float y = (std::abs(joystick_y_) > dead_zone) ? joystick_y_ : 0.0f;
    int16_t pitch_bend = CalculatePitchBend(y);
    if (pitch_bend != last_pitch_bend_value_) {
        // Pitch bend changed - send MIDI pitch bend message
        // Pitch bend uses 14-bit value (0-16383), 8192 is center
        // This bends all currently playing notes chromatically, regardless of scale mode
        // Split into LSB (7 bits) and MSB (7 bits)
        if (pending_write_pos_ < pending_events_.size()) {
            MidiEvent& event = pending_events_[pending_write_pos_];
            event.type = static_cast<uint8_t>(MidiEvent::Type::PITCH_BEND);
            event.channel = 0;
            event.data1 = pitch_bend & 0x7F;        // LSB (bits 0-6)
            event.data2 = (pitch_bend >> 7) & 0x7F; // MSB (bits 7-13)
            event.timestamp = 0;
            
            pending_write_pos_++;
            if (pending_write_pos_ >= pending_events_.size()) {
                pending_write_pos_ = 0;
            }
        }
        last_pitch_bend_value_ = pitch_bend;
    }
    
    // Process mod wheel (X axis: left/right)
    // Apply dead zone to avoid noise when joystick is centered
    // Center and left = 0, only right gives mod wheel value
    float x = (std::abs(joystick_x_) > dead_zone) ? joystick_x_ : 0.0f;
    uint8_t mod_wheel = CalculateModWheel(x);
    if (mod_wheel != last_mod_wheel_value_) {
        // Mod wheel changed - send MIDI CC 1 (Modulation Wheel)
        if (pending_write_pos_ < pending_events_.size()) {
            MidiEvent& event = pending_events_[pending_write_pos_];
            event.type = static_cast<uint8_t>(MidiEvent::Type::CONTROL_CHANGE);
            event.channel = 0;
            event.data1 = 1;  // CC 1 = Modulation Wheel
            event.data2 = mod_wheel;  // Value 0-127
            event.timestamp = 0;
            
            pending_write_pos_++;
            if (pending_write_pos_ >= pending_events_.size()) {
                pending_write_pos_ = 0;
            }
        }
        last_mod_wheel_value_ = mod_wheel;
    }
}

int16_t PianoInput::CalculatePitchBend(float joystick_y) const {
    // Map joystick Y (-0.5 to 0.5) to pitch bend (0 to 16383)
    // Center (0.0) = 8192 (no bend)
    // Up (positive Y, 0.5) = 16383 (max bend up)
    // Down (negative Y, -0.5) = 0 (max bend down)
    // MIDI pitch bend: 0 = max down, 8192 = center, 16383 = max up
    
    // Clamp joystick value to [-0.5, 0.5] for full range
    if (joystick_y > 0.5f) joystick_y = 0.5f;
    if (joystick_y < -0.5f) joystick_y = -0.5f;
    
    // Map from [-0.5, 0.5] to [0, 16383] with center at 8192
    // Scale by 2.0 to map [-0.5, 0.5] to [-1.0, 1.0], then multiply by 8192
    int16_t pitch_bend = static_cast<int16_t>(8192 + (joystick_y * 2.0f * 8192));
    
    // Clamp to valid range
    if (pitch_bend < 0) pitch_bend = 0;
    if (pitch_bend > 16383) pitch_bend = 16383;
    
    return pitch_bend;
}

uint8_t PianoInput::CalculateModWheel(float joystick_x) const {
    // Map joystick X (-0.5 to 0.5) to mod wheel (0 to 127)
    // Center (0.0) = 0 (no modulation)
    // Right (positive X, 0.5) or Left (negative X, -0.5) = 127 (max modulation)
    // Both directions give mod wheel output, maxes out at ±0.5
    
    // Clamp joystick value to [-0.5, 0.5] for full range
    if (joystick_x > 0.5f) joystick_x = 0.5f;
    if (joystick_x < -0.5f) joystick_x = -0.5f;
    
    // Use absolute value so both left and right give mod wheel output
    // Scale by 2.0 to map [-0.5, 0.5] to [0.0, 1.0], then scale to [0, 127]
    float abs_x = std::abs(joystick_x);  // [0.0, 0.5]
    float normalized = abs_x * 2.0f;     // [0.0, 1.0]
    uint8_t mod_wheel = static_cast<uint8_t>(normalized * 127.0f);
    
    // Clamp to valid range
    if (mod_wheel > 127) mod_wheel = 127;
    
    return mod_wheel;
}

uint8_t PianoInput::GetMidiNote(int button_index) const {
    if (button_index < 0 || button_index >= 7) {
        return 60;  // Default to C4
    }
    
    // Physical layout: 4 white keys (bottom row) + 3 black keys (middle row)
    // Physical left-to-right order: White0, Black0, White1, Black1, White2, Black2, White3
    // Button indices in physical order: 0, 4, 1, 5, 2, 6, 3
    //
    // Map to 7 consecutive semitones starting from root (simple and clean):
    // Physical position 0 (White0, button 0) = root + 0
    // Physical position 1 (Black0, button 4) = root + 1
    // Physical position 2 (White1, button 1) = root + 2
    // Physical position 3 (Black1, button 5) = root + 3
    // Physical position 4 (White2, button 2) = root + 4
    // Physical position 5 (Black2, button 6) = root + 5
    // Physical position 6 (White3, button 3) = root + 6
    //
    // Button index -> semitone interval mapping:
    static const uint8_t BUTTON_TO_INTERVAL[7] = {0, 2, 4, 6, 1, 3, 5};
    
    // Base note is C4 (60) + the selected key root (0-11)
    uint8_t base_note = 60 + current_key_.root_note;
    
    // Calculate MIDI note: base + interval for this button
    uint8_t midi_note = base_note + BUTTON_TO_INTERVAL[button_index];
    
    // Clamp to valid MIDI range (0-127)
    if (midi_note > 127) {
        midi_note = 127;
    }
    
    return midi_note;
}

uint8_t PianoInput::GetScaleNote(int button_index) const {
    if (button_index < 0 || button_index >= 7) {
        return 60;  // Default to C4
    }
    
    // Use ChordEngine to get the scale degree note for this button
    uint8_t root_midi_note;
    chord_engine_.GetButtonMapping(current_key_, button_index, &root_midi_note);
    
    return root_midi_note;
}

std::vector<uint8_t> PianoInput::GetActiveNotes() const {
    std::vector<uint8_t> active;
    
    if (!initialized_) return active;
    
    for (int i = 0; i < 7; i++) {
        if (current_button_states_[i]) {
            uint8_t midi_note;
            if (play_mode_ == PlayMode::SCALE) {
                midi_note = GetScaleNote(i);
            } else {
                // In chromatic mode, Key setting determines the starting note
                midi_note = GetMidiNote(i);
            }
            
            // Apply octave shift if available (for display purposes)
            if (octave_shift_) {
                midi_note = octave_shift_->ApplyShift(midi_note);
            }
            active.push_back(midi_note);
        }
    }
    
    // Sort notes (lowest to highest)
    std::sort(active.begin(), active.end());
    
    return active;
}

void PianoInput::SetKey(MusicalKey key) {
    current_key_ = key;
    mode_setting_value_ = static_cast<int>(key.mode);
    key_root_setting_value_ = key.root_note;  // Sync setting value
}

void PianoInput::SetPlayMode(PlayMode mode) {
    play_mode_ = mode;
    play_mode_setting_value_ = static_cast<int>(mode);
}

void PianoInput::InitializeSettings() {
    // Setting 0: Play Mode (Chromatic/Scale)
    settings_[0].name = "Mode";
    settings_[0].type = SettingType::ENUM;
    settings_[0].value_ptr = &play_mode_setting_value_;
    settings_[0].min_value = 0;
    settings_[0].max_value = 1;
    settings_[0].step_size = 1.0f;
    settings_[0].enum_options = play_mode_names;
    settings_[0].enum_count = 2;
    settings_[0].on_change_callback = nullptr;
    
    // Setting 1: Key Root (C through B) - use ENUM for note names
    settings_[1].name = "Key";
    settings_[1].type = SettingType::ENUM;
    settings_[1].value_ptr = &key_root_setting_value_;
    settings_[1].min_value = 0;
    settings_[1].max_value = 11;
    settings_[1].step_size = 1.0f;
    settings_[1].enum_options = note_names;
    settings_[1].enum_count = 12;
    settings_[1].on_change_callback = nullptr;
    
    // Setting 2: Scale Mode (Ionian, Dorian, etc.) - only relevant in Scale mode
    settings_[2].name = "Scale";
    settings_[2].type = SettingType::ENUM;
    settings_[2].value_ptr = &mode_setting_value_;
    settings_[2].min_value = 0;
    settings_[2].max_value = 6;
    settings_[2].step_size = 1.0f;
    settings_[2].enum_options = mode_names;
    settings_[2].enum_count = 7;
    settings_[2].on_change_callback = nullptr;
}

int PianoInput::GetSettingCount() const {
    return SETTING_COUNT;
}

const PluginSetting* PianoInput::GetSetting(int index) const {
    if (index < 0 || index >= SETTING_COUNT) {
        return nullptr;
    }
    return &settings_[index];
}

void PianoInput::OnSettingChanged(int setting_index) {
    switch (setting_index) {
        case 0:  // Play Mode
            play_mode_ = static_cast<PlayMode>(play_mode_setting_value_);
            break;
        case 1:  // Key Root
            current_key_.root_note = static_cast<uint8_t>(key_root_setting_value_);
            break;
        case 2:  // Scale Mode
            current_key_.mode = static_cast<MusicalMode>(mode_setting_value_);
            break;
    }
}

void PianoInput::EnsureDefaultActivation() {
    if (!track_) return;
    
    // Check if any other exclusive INPUT plugin is active
    // (FX and instrument plugins are separate and don't affect this)
    bool any_other_active = false;
    const auto& plugins = track_->GetInputPlugins();  // Only input plugins
    for (const auto& plugin : plugins) {
        if (plugin.get() == this) continue;  // Skip ourselves
        if (plugin && plugin->IsExclusive() && plugin->IsActive()) {
            any_other_active = true;
            break;
        }
    }
    
    // If no other exclusive input plugin is active, ensure we're active (default behavior)
    if (!any_other_active && !active_) {
        active_ = true;
    }
}

} // namespace OpenChord

