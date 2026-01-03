#include "drum_pad_input.h"
#include "../../core/io/input_manager.h"
#include <cstring>
#include <algorithm>

namespace OpenChord {

DrumPadInput::DrumPadInput()
    : input_manager_(nullptr)
    , active_(false)  // Inactive by default
    , initialized_(false)
    , pending_read_pos_(0)
    , pending_write_pos_(0)
{
    std::memset(prev_button_states_, false, sizeof(prev_button_states_));
    std::memset(current_button_states_, false, sizeof(current_button_states_));
    pending_events_.resize(128);
}

DrumPadInput::~DrumPadInput() {
}

void DrumPadInput::Init() {
    if (!input_manager_) return;
    
    active_ = false;  // Start inactive
    initialized_ = true;
    std::memset(prev_button_states_, false, sizeof(prev_button_states_));
    std::memset(current_button_states_, false, sizeof(current_button_states_));
    pending_read_pos_ = 0;
    pending_write_pos_ = 0;
}

void DrumPadInput::Process(const float* const* in, float* const* out, size_t size) {
    // This plugin doesn't process audio directly
    // It only generates MIDI events
    (void)in;
    (void)out;
    (void)size;
}

void DrumPadInput::Update() {
    if (!initialized_ || !active_ || !input_manager_) return;
    
    // Process button state changes
    ProcessButtons();
}

void DrumPadInput::UpdateUI() {
    // UI updates handled by main UI system
}

void DrumPadInput::HandleEncoder(int encoder, float delta) {
    // Could be used for velocity control or drum kit selection
    // For now, no encoder handling
}

void DrumPadInput::HandleButton(int button, bool pressed) {
    // Buttons are processed via ProcessButtons() which polls state directly
    // This method could be used for system buttons if needed
}

void DrumPadInput::HandleJoystick(float x, float y) {
    // Joystick could be used for velocity control or effects
    // For now, no joystick handling
}

void DrumPadInput::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    // Save state: active flag
    *size = sizeof(bool);
    *reinterpret_cast<bool*>(buffer) = active_;
}

void DrumPadInput::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(bool)) return;
    
    active_ = *reinterpret_cast<const bool*>(buffer);
}

size_t DrumPadInput::GetStateSize() const {
    return sizeof(bool);
}

void DrumPadInput::GenerateMIDI(MidiEvent* events, size_t* count, size_t max_events) {
    *count = 0;
    
    if (!active_ || !initialized_ || !events) return;
    
    // Read from pending events buffer
    while (*count < max_events && pending_read_pos_ != pending_write_pos_) {
        events[*count] = pending_events_[pending_read_pos_];
        pending_read_pos_ = (pending_read_pos_ + 1) % pending_events_.size();
        (*count)++;
    }
}

void DrumPadInput::ProcessMIDI(const MidiEvent* events, size_t count) {
    // This plugin generates MIDI, doesn't process incoming MIDI
    // Could be enhanced to transform incoming MIDI if needed
}

void DrumPadInput::ProcessButtons() {
    if (!input_manager_) return;
    
    ButtonInputHandler& buttons = input_manager_->GetButtons();
    
    for (int i = 0; i < 7; i++) {
        MusicalButton button = static_cast<MusicalButton>(i);
        bool current_pressed = buttons.IsMusicalButtonPressed(button);
        bool prev_pressed = prev_button_states_[i];
        
        // Update current state
        current_button_states_[i] = current_pressed;
        
        if (current_pressed != prev_pressed) {
            uint8_t drum_note = GetDrumNote(i);
            
            if (current_pressed && !prev_pressed) {
                // Button pressed: send NOTE_ON on drum channel
                size_t next_write = (pending_write_pos_ + 1) % pending_events_.size();
                if (next_write != pending_read_pos_) {
                    MidiEvent& event = pending_events_[pending_write_pos_];
                    event.type = static_cast<uint8_t>(MidiEvent::Type::NOTE_ON);
                    event.channel = DRUM_CHANNEL;  // Channel 10 (0-based = 9) for drums
                    event.data1 = drum_note;
                    event.data2 = 100;  // Default velocity (can be enhanced with velocity sensitivity)
                    event.timestamp = 0;
                    
                    pending_write_pos_ = next_write;
                }
            } else if (!current_pressed && prev_pressed) {
                // Button released: send NOTE_OFF on drum channel
                // Note: Some drum sounds are one-shot and don't need NOTE_OFF,
                // but we send it for completeness and compatibility
                size_t next_write = (pending_write_pos_ + 1) % pending_events_.size();
                if (next_write != pending_read_pos_) {
                    MidiEvent& event = pending_events_[pending_write_pos_];
                    event.type = static_cast<uint8_t>(MidiEvent::Type::NOTE_OFF);
                    event.channel = DRUM_CHANNEL;  // Channel 10 (0-based = 9) for drums
                    event.data1 = drum_note;
                    event.data2 = 0;
                    event.timestamp = 0;
                    
                    pending_write_pos_ = next_write;
                }
            }
            
            prev_button_states_[i] = current_pressed;
        }
    }
}

uint8_t DrumPadInput::GetDrumNote(int button_index) const {
    if (button_index < 0 || button_index >= 7) {
        return 36;  // Default to Kick (C1)
    }
    
    // Standard GM (General MIDI) drum mapping
    // Physical order: White0, White1, White2, White3, Black0, Black1, Black2
    // Maps to: Kick, Snare, Hi-Hat Closed, Hi-Hat Open, Crash, Ride, Tom
    static const uint8_t DRUM_NOTES[7] = {
        36,  // C1 - Kick
        38,  // D1 - Snare
        40,  // E1 - Hi-Hat Closed
        42,  // F1 - Hi-Hat Open
        37,  // C#1 - Crash Cymbal
        39,  // D#1 - Ride Cymbal
        41   // F#1 - Tom
    };
    
    return DRUM_NOTES[button_index];
}

std::vector<uint8_t> DrumPadInput::GetActiveNotes() const {
    std::vector<uint8_t> active;
    
    if (!initialized_) return active;
    
    for (int i = 0; i < 7; i++) {
        if (current_button_states_[i]) {
            active.push_back(GetDrumNote(i));
        }
    }
    
    // Sort notes (lowest to highest)
    std::sort(active.begin(), active.end());
    
    return active;
}

} // namespace OpenChord





