#include "../include/system_interface.h"
#include "../include/track_interface.h"
#include <cstring>
#include <cstdio>

namespace OpenChord {

OpenChordSystem::OpenChordSystem() 
    : active_track_(0), current_play_mode_(nullptr), tempo_(120.0f),
      time_signature_numerator_(4), time_signature_denominator_(4),
      sample_rate_(48000.0f), buffer_size_(4), sample_clock_(0) {
    
    // Initialize tracks
    for (int i = 0; i < MAX_TRACKS; i++) {
        tracks_.push_back(std::make_unique<Track>());
    }
}

OpenChordSystem::~OpenChordSystem() {
}

void OpenChordSystem::Init() {
    // Initialize all tracks
    for (auto& track : tracks_) {
        track->Init();
    }
    
    // Set default track names
    for (int i = 0; i < tracks_.size(); i++) {
        char name[32];
        snprintf(name, sizeof(name), "Track %d", i + 1);
        tracks_[i]->SetName(name);
    }
    
    // Set active track to 0
    SetActiveTrack(0);
}

void OpenChordSystem::Process(float* in, float* out, size_t size) {
    // Clear output buffer
    for (size_t i = 0; i < size; i++) {
        out[i] = 0.0f;
    }
    
    // Process all tracks
    ProcessTracks(in, out, size);
    
    // Update sample clock
    UpdateSampleClock();
}

void OpenChordSystem::Update() {
    // Update all tracks
    for (auto& track : tracks_) {
        track->Update();
    }
    
    // Update PlayMode if active
    if (current_play_mode_ && current_play_mode_->IsActive()) {
        current_play_mode_->Update();
    }
}

Track* OpenChordSystem::GetTrack(int index) {
    if (index < 0 || index >= tracks_.size()) return nullptr;
    return tracks_[index].get();
}

const Track* OpenChordSystem::GetTrack(int index) const {
    if (index < 0 || index >= tracks_.size()) return nullptr;
    return tracks_[index].get();
}

void OpenChordSystem::SetActiveTrack(int track) {
    if (track < 0 || track >= tracks_.size()) return;
    active_track_ = track;
}

int OpenChordSystem::GetActiveTrack() const {
    return active_track_;
}

int OpenChordSystem::GetTrackCount() const {
    return static_cast<int>(tracks_.size());
}

void OpenChordSystem::SetPlayMode(std::unique_ptr<IPlayModePlugin> play_mode) {
    // Exit current PlayMode if active
    if (current_play_mode_ && current_play_mode_->IsActive()) {
        current_play_mode_->ExitMode();
    }
    
    current_play_mode_ = std::move(play_mode);
    
    // Enter new PlayMode if provided
    if (current_play_mode_) {
        current_play_mode_->EnterMode();
    }
}

void OpenChordSystem::ClearPlayMode() {
    if (current_play_mode_ && current_play_mode_->IsActive()) {
        current_play_mode_->ExitMode();
    }
    current_play_mode_.reset();
}

IPlayModePlugin* OpenChordSystem::GetCurrentPlayMode() const {
    return current_play_mode_.get();
}

bool OpenChordSystem::IsPlayModeActive() const {
    return current_play_mode_ && current_play_mode_->IsActive();
}

void OpenChordSystem::UpdateUI() {
    // Update all tracks UI
    for (auto& track : tracks_) {
        track->UpdateUI();
    }
    
    // Update PlayMode UI if active
    if (current_play_mode_ && current_play_mode_->IsActive()) {
        current_play_mode_->UpdateUI();
    }
}

void OpenChordSystem::HandleEncoder(int encoder, float delta) {
    // Check if PlayMode wants to handle this
    if (current_play_mode_ && current_play_mode_->IsActive()) {
        if (current_play_mode_->OverrideEncoder(encoder, delta)) {
            return;
        }
    }
    
    // Route to active track
    if (active_track_ >= 0 && active_track_ < tracks_.size()) {
        tracks_[active_track_]->HandleEncoder(encoder, delta);
    }
}

void OpenChordSystem::HandleButton(int button, bool pressed) {
    // Check if PlayMode wants to handle this
    if (current_play_mode_ && current_play_mode_->IsActive()) {
        if (current_play_mode_->OverrideButton(button, pressed)) {
            return;
        }
    }
    
    // Route to active track
    if (active_track_ >= 0 && active_track_ < tracks_.size()) {
        tracks_[active_track_]->HandleButton(button, pressed);
    }
}

void OpenChordSystem::HandleJoystick(float x, float y) {
    // Check if PlayMode wants to handle this
    if (current_play_mode_ && current_play_mode_->IsActive()) {
        if (current_play_mode_->OverrideJoystick(x, y)) {
            return;
        }
    }
    
    // Route to active track
    if (active_track_ >= 0 && active_track_ < tracks_.size()) {
        tracks_[active_track_]->HandleJoystick(x, y);
    }
}

void OpenChordSystem::ProcessMIDI(const MidiEvent* events, size_t count) {
    // Process MIDI through all tracks
    for (auto& track : tracks_) {
        track->ProcessMIDI(events, count);
    }
}

void OpenChordSystem::SendMIDI(const MidiEvent* events, size_t count) {
    // This would send MIDI to external devices
    // Implementation depends on hardware MIDI interface
}

void OpenChordSystem::SetTempo(float bpm) {
    tempo_ = bpm;
}

float OpenChordSystem::GetTempo() const {
    return tempo_;
}

void OpenChordSystem::SetTimeSignature(int numerator, int denominator) {
    time_signature_numerator_ = numerator;
    time_signature_denominator_ = denominator;
}

void OpenChordSystem::GetTimeSignature(int* numerator, int* denominator) const {
    if (numerator) *numerator = time_signature_numerator_;
    if (denominator) *denominator = time_signature_denominator_;
}

void OpenChordSystem::SaveProject(const char* filename) {
    // TODO: Implement project saving
}

void OpenChordSystem::LoadProject(const char* filename) {
    // TODO: Implement project loading
}

void OpenChordSystem::NewProject() {
    // Reset all tracks to default state
    for (auto& track : tracks_) {
        track->Init();
    }
}

void OpenChordSystem::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
}

float OpenChordSystem::GetSampleRate() const {
    return sample_rate_;
}

void OpenChordSystem::SetBufferSize(size_t buffer_size) {
    buffer_size_ = buffer_size;
}

size_t OpenChordSystem::GetBufferSize() const {
    return buffer_size_;
}

void OpenChordSystem::ProcessTracks(float* in, float* out, size_t size) {
    // Process each track and mix to output
    for (auto& track : tracks_) {
        track->Process(in, out, size);
    }
}

void OpenChordSystem::UpdateSampleClock() {
    sample_clock_ += buffer_size_;
}

} // namespace OpenChord 