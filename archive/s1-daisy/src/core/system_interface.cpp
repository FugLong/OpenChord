#include "system_interface.h"
#include "audio/volume_manager.h"
#include "audio/audio_engine.h"
#include "midi/octave_shift.h"
#include <cstring>

namespace OpenChord {

OpenChordSystem::OpenChordSystem()
    : volume_manager_(nullptr)
    , octave_shift_(nullptr)
    , active_track_(0)
    , tempo_(120.0f)
    , time_signature_numerator_(4)
    , time_signature_denominator_(4)
    , sample_rate_(48000.0f)
    , buffer_size_(4)
    , sample_clock_(0)
{
    // Initialize tracks
    tracks_.reserve(MAX_TRACKS);
    for (int i = 0; i < MAX_TRACKS; i++) {
        tracks_.push_back(std::make_unique<Track>());
    }
}

OpenChordSystem::~OpenChordSystem() {
}

void OpenChordSystem::Init() {
    // Initialize all tracks
    for (auto& track : tracks_) {
        if (track) {
            track->Init();
            // Set octave shift if available
            if (octave_shift_) {
                track->SetOctaveShift(octave_shift_);
            }
        }
    }
    
    // Set default track names
    if (tracks_.size() > 0 && tracks_[0]) {
        tracks_[0]->SetName("Track 1");
    }
    if (tracks_.size() > 1 && tracks_[1]) {
        tracks_[1]->SetName("Track 2");
    }
    if (tracks_.size() > 2 && tracks_[2]) {
        tracks_[2]->SetName("Track 3");
    }
    if (tracks_.size() > 3 && tracks_[3]) {
        tracks_[3]->SetName("Track 4");
    }
    
    // Reset sample clock
    sample_clock_ = 0;
}

void OpenChordSystem::Process(const float* const* in, float* const* out, size_t size) {
    ProcessTracks(in, out, size);
    UpdateSampleClock();
}

void OpenChordSystem::Update() {
    // Update all tracks
    for (auto& track : tracks_) {
        if (track) {
            track->Update();
        }
    }
    
    // Update PlayMode if active
    if (current_play_mode_ && current_play_mode_->IsActive()) {
        current_play_mode_->Update();
    }
}

Track* OpenChordSystem::GetTrack(int index) {
    if (index >= 0 && index < static_cast<int>(tracks_.size())) {
        return tracks_[index].get();
    }
    return nullptr;
}

const Track* OpenChordSystem::GetTrack(int index) const {
    if (index >= 0 && index < static_cast<int>(tracks_.size())) {
        return tracks_[index].get();
    }
    return nullptr;
}

void OpenChordSystem::SetActiveTrack(int track) {
    if (track >= 0 && track < static_cast<int>(tracks_.size())) {
        active_track_ = track;
    }
}

int OpenChordSystem::GetActiveTrack() const {
    return active_track_;
}

int OpenChordSystem::GetTrackCount() const {
    return static_cast<int>(tracks_.size());
}

void OpenChordSystem::SetPlayMode(std::unique_ptr<IPlayModePlugin> play_mode) {
    if (current_play_mode_) {
        current_play_mode_->ExitMode();
    }
    current_play_mode_ = std::move(play_mode);
    if (current_play_mode_) {
        current_play_mode_->EnterMode();
    }
}

void OpenChordSystem::ClearPlayMode() {
    if (current_play_mode_) {
        current_play_mode_->ExitMode();
        current_play_mode_.reset();
    }
}

IPlayModePlugin* OpenChordSystem::GetCurrentPlayMode() const {
    return current_play_mode_.get();
}

bool OpenChordSystem::IsPlayModeActive() const {
    return current_play_mode_ != nullptr && current_play_mode_->IsActive();
}

void OpenChordSystem::UpdateUI() {
    // Update active track UI
    Track* active = GetTrack(active_track_);
    if (active) {
        active->UpdateUI();
    }
    
    // Update PlayMode UI if active
    if (current_play_mode_ && current_play_mode_->IsActive()) {
        current_play_mode_->UpdateUI();
    }
}

void OpenChordSystem::HandleEncoder(int encoder, float delta) {
    // Check PlayMode override
    if (current_play_mode_ && current_play_mode_->IsActive()) {
        if (current_play_mode_->OverrideEncoder(encoder, delta)) {
            return;
        }
    }
    
    // Route to active track
    Track* active = GetTrack(active_track_);
    if (active) {
        active->HandleEncoder(encoder, delta);
    }
}

void OpenChordSystem::HandleButton(int button, bool pressed) {
    // Check PlayMode override
    if (current_play_mode_ && current_play_mode_->IsActive()) {
        if (current_play_mode_->OverrideButton(button, pressed)) {
            return;
        }
    }
    
    // Route to active track
    Track* active = GetTrack(active_track_);
    if (active) {
        active->HandleButton(button, pressed);
    }
}

void OpenChordSystem::HandleJoystick(float x, float y) {
    // Check PlayMode override
    if (current_play_mode_ && current_play_mode_->IsActive()) {
        if (current_play_mode_->OverrideJoystick(x, y)) {
            return;
        }
    }
    
    // Route to active track
    Track* active = GetTrack(active_track_);
    if (active) {
        active->HandleJoystick(x, y);
    }
}

void OpenChordSystem::ProcessMIDI(const MidiEvent* events, size_t count) {
    // Route MIDI to active track
    Track* active = GetTrack(active_track_);
    if (active) {
        active->ProcessMIDI(events, count);
    }
}

void OpenChordSystem::SendMIDI(const MidiEvent* events, size_t count) {
    // Store MIDI events for external routing
    midi_buffer_.clear();
    midi_buffer_.insert(midi_buffer_.end(), events, events + count);
}

void OpenChordSystem::SetTempo(float bpm) {
    tempo_ = bpm;
    if (tempo_ < 20.0f) tempo_ = 20.0f;
    if (tempo_ > 300.0f) tempo_ = 300.0f;
}

float OpenChordSystem::GetTempo() const {
    return tempo_;
}

void OpenChordSystem::SetTimeSignature(int numerator, int denominator) {
    time_signature_numerator_ = numerator;
    time_signature_denominator_ = denominator;
    if (time_signature_numerator_ < 1) time_signature_numerator_ = 1;
    if (time_signature_denominator_ < 1) time_signature_denominator_ = 1;
}

void OpenChordSystem::GetTimeSignature(int* numerator, int* denominator) const {
    if (numerator) *numerator = time_signature_numerator_;
    if (denominator) *denominator = time_signature_denominator_;
}

void OpenChordSystem::SaveProject(const char* filename) {
    // TODO: Implement project saving
    (void)filename;
}

void OpenChordSystem::LoadProject(const char* filename) {
    // TODO: Implement project loading
    (void)filename;
}

void OpenChordSystem::NewProject() {
    // Reset all tracks
    for (auto& track : tracks_) {
        if (track) {
            track->Init();
        }
    }
    active_track_ = 0;
    tempo_ = 120.0f;
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

void OpenChordSystem::ProcessTracks(const float* const* in, float* const* out, size_t size) {
    // Initialize output to silence
    for (size_t i = 0; i < size; i++) {
        out[0][i] = 0.0f;
        out[1][i] = 0.0f;
    }
    
    // Allocate temporary buffers for track processing
    float track_buffer[2][64];  // Assume max 64 samples per block
    size_t process_size = size > 64 ? 64 : size;
    
    // Process all non-soloed tracks, or soloed tracks if any exist
    bool has_solo = false;
    for (const auto& track : tracks_) {
        if (track && track->IsSoloed()) {
            has_solo = true;
            break;
        }
    }
    
    // Mix all tracks
    for (const auto& track : tracks_) {
        if (!track) continue;
        
        // Skip muted tracks
        if (track->IsMuted()) continue;
        
        // If any track is soloed, only process soloed tracks
        if (has_solo && !track->IsSoloed()) continue;
        
        // Clear track buffer
        for (size_t i = 0; i < process_size; i++) {
            track_buffer[0][i] = 0.0f;
            track_buffer[1][i] = 0.0f;
        }
        
        // Process track (instruments generate from silence, so pass nullptr for input)
        const float* track_in[2] = {nullptr, nullptr};
        float* track_out[2] = {track_buffer[0], track_buffer[1]};
        track->Process(track_in, track_out, process_size);
        
        // Mix into output
        for (size_t i = 0; i < process_size && i < size; i++) {
            out[0][i] += track_buffer[0][i];
            out[1][i] += track_buffer[1][i];
        }
    }
    
    // Apply master volume/gain scaling to prevent clipping
    // Simple soft clipping
    for (size_t i = 0; i < size; i++) {
        float left = out[0][i];
        float right = out[1][i];
        
        if (left > 1.0f) left = 1.0f;
        if (left < -1.0f) left = -1.0f;
        if (right > 1.0f) right = 1.0f;
        if (right < -1.0f) right = -1.0f;
        
        out[0][i] = left;
        out[1][i] = right;
    }
}

void OpenChordSystem::UpdateSampleClock() {
    sample_clock_ += buffer_size_;
}

void OpenChordSystem::SetOctaveShift(OctaveShift* octave_shift) {
    octave_shift_ = octave_shift;
    // Update all tracks
    for (auto& track : tracks_) {
        if (track) {
            track->SetOctaveShift(octave_shift_);
        }
    }
}

} // namespace OpenChord

