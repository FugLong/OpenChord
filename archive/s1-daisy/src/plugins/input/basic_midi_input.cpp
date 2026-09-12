#include "basic_midi_input.h"
#include <cstring>

namespace OpenChord {

BasicMidiInput::BasicMidiInput() 
    : active_(true), buffer_read_pos_(0), buffer_write_pos_(0) {
    midi_buffer_.resize(1024);  // Circular buffer for MIDI events
}

BasicMidiInput::~BasicMidiInput() {
}

void BasicMidiInput::Init() {
    active_ = true;
    buffer_read_pos_ = 0;
    buffer_write_pos_ = 0;
}

void BasicMidiInput::Process(const float* const* in, float* const* out, size_t size) {
    // This plugin doesn't process audio directly
    // It only handles MIDI events
    (void)in;
    (void)out;
    (void)size;
}

void BasicMidiInput::Update() {
    // Nothing to update for this simple plugin
}

void BasicMidiInput::UpdateUI() {
    // Simple UI update - could show MIDI activity indicator
}

void BasicMidiInput::HandleEncoder(int encoder, float delta) {
    // No encoder handling for this plugin
}

void BasicMidiInput::HandleButton(int button, bool pressed) {
    // No button handling for this plugin
}

void BasicMidiInput::HandleJoystick(float x, float y) {
    // No joystick handling for this plugin
}

void BasicMidiInput::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    // Save simple state
    bool state = active_;
    std::memcpy(buffer, &state, sizeof(bool));
    *size = sizeof(bool);
}

void BasicMidiInput::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(bool)) return;
    
    // Load simple state
    bool state;
    std::memcpy(&state, buffer, sizeof(bool));
    active_ = state;
}

size_t BasicMidiInput::GetStateSize() const {
    return sizeof(bool);
}

void BasicMidiInput::GenerateMIDI(MidiEvent* events, size_t* count, size_t max_events) {
    *count = 0;
    
    if (!active_) return;
    
    // Read events from circular buffer
    while (*count < max_events && buffer_read_pos_ != buffer_write_pos_) {
        events[*count] = midi_buffer_[buffer_read_pos_];
        buffer_read_pos_ = (buffer_read_pos_ + 1) % midi_buffer_.size();
        (*count)++;
    }
}

void BasicMidiInput::ProcessMIDI(const MidiEvent* events, size_t count) {
    if (!active_) return;
    
    // Write events to circular buffer
    for (size_t i = 0; i < count; i++) {
        size_t next_write = (buffer_write_pos_ + 1) % midi_buffer_.size();
        if (next_write != buffer_read_pos_) {  // Don't overwrite unread data
            midi_buffer_[buffer_write_pos_] = events[i];
            buffer_write_pos_ = next_write;
        }
    }
}

} // namespace OpenChord 