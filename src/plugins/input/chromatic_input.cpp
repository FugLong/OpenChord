#include "chromatic_input.h"
#include "../../core/io/input_manager.h"
#include "../../core/midi/octave_shift.h"
#include "../../core/tracks/track_interface.h"
#include <cstring>
#include <algorithm>

namespace OpenChord {

ChromaticInput::ChromaticInput()
    : input_manager_(nullptr)
    , octave_shift_(nullptr)
    , track_(nullptr)
    , active_(true)
    , initialized_(false)
    , pending_read_pos_(0)
    , pending_write_pos_(0)
{
    std::memset(prev_button_states_, false, sizeof(prev_button_states_));
    std::memset(current_button_states_, false, sizeof(current_button_states_));
    pending_events_.resize(128);
}

ChromaticInput::~ChromaticInput() {
}

void ChromaticInput::Init() {
    if (!input_manager_) return;
    
    active_ = true;
    initialized_ = true;
    std::memset(prev_button_states_, false, sizeof(prev_button_states_));
    pending_read_pos_ = 0;
    pending_write_pos_ = 0;
}

void ChromaticInput::Process(float* in, float* out, size_t size) {
    // No audio processing
}

void ChromaticInput::Update() {
    if (!initialized_ || !active_ || !input_manager_) return;
    
    // Only process if we're actually active (not just enabled)
    // The track system will call GenerateMIDI which checks IsActive()
    ProcessButtons();
}

void ChromaticInput::UpdateUI() {
    // No UI updates needed
}

void ChromaticInput::HandleEncoder(int encoder, float delta) {
    // No encoder handling
}

void ChromaticInput::HandleButton(int button, bool pressed) {
    // Buttons processed via ProcessButtons()
}

void ChromaticInput::HandleJoystick(float x, float y) {
    // No joystick handling
}

void ChromaticInput::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    *size = sizeof(bool);  // Just active state
    *reinterpret_cast<bool*>(buffer) = active_;
}

void ChromaticInput::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(bool)) return;
    active_ = *reinterpret_cast<const bool*>(buffer);
}

size_t ChromaticInput::GetStateSize() const {
    return sizeof(bool);
}

bool ChromaticInput::IsActive() const {
    // Chromatic input is only active if:
    // 1. Our active flag is true
    // 2. No other input plugin with higher priority is active
    if (!active_ || !initialized_) {
        return false;
    }
    
    // Check if any other input plugin is active (with higher priority)
    if (track_) {
        const auto& plugins = track_->GetInputPlugins();
        for (const auto& plugin : plugins) {
            if (plugin.get() == this) continue;  // Skip ourselves
            if (plugin && plugin->IsActive() && plugin->GetPriority() < GetPriority()) {
                // Another plugin with higher priority is active
                return false;
            }
        }
    }
    
    return true;
}

void ChromaticInput::ProcessMIDI(const MidiEvent* events, size_t count) {
    // Chromatic input doesn't process incoming MIDI
}

void ChromaticInput::GenerateMIDI(MidiEvent* events, size_t* count, size_t max_events) {
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

void ChromaticInput::ProcessButtons() {
    if (!input_manager_) return;
    
    auto& button_handler = input_manager_->GetButtons();
    
    for (int i = 0; i < 7; i++) {
        MusicalButton button = static_cast<MusicalButton>(i);
        bool current_pressed = button_handler.IsMusicalButtonPressed(button);
        bool prev_pressed = prev_button_states_[i];
        
        // Update current state
        current_button_states_[i] = current_pressed;
        
        if (current_pressed != prev_pressed) {
            uint8_t midi_note = GetMidiNote(i);
            // Note: Octave shift is applied in main.cpp to all MIDI events before sending
            
            if (current_pressed && !prev_pressed) {
                // Button pressed: send NOTE_ON
                // Note: Octave shift is applied in main.cpp to all MIDI events
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
                // Note: Octave shift is applied in main.cpp to all MIDI events
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

uint8_t ChromaticInput::GetMidiNote(int button_index) const {
    if (button_index < 0 || button_index >= 7) {
        return 60;  // Default to C4
    }
    
    // Default MIDI notes for each button (C4=60 base)
    // Physical order: White0=C, White1=D, White2=E, White3=F, Black0=C#, Black1=D#, Black2=F#
    static const uint8_t BASE_NOTES[7] = {60, 62, 64, 65, 61, 63, 66};  // C, D, E, F, C#, D#, F#
    return BASE_NOTES[button_index];
}

std::vector<uint8_t> ChromaticInput::GetActiveNotes() const {
    std::vector<uint8_t> active;
    
    if (!initialized_) return active;
    
    for (int i = 0; i < 7; i++) {
        if (current_button_states_[i]) {
            uint8_t midi_note = GetMidiNote(i);
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

} // namespace OpenChord

