#pragma once

#include "tracks/track_interface.h"
#include "plugin_interface.h"
#include "audio/volume_interface.h"
#include "audio/audio_engine.h"
#include <vector>
#include <memory>

// Forward declarations
class IO;

namespace OpenChord {

class OctaveShift;

/**
 * Main system interface for OpenChord
 */
class OpenChordSystem {
public:
    static constexpr int MAX_TRACKS = 4;
    static constexpr int MAX_SCENES = 8;

    OpenChordSystem();
    ~OpenChordSystem();

    // System lifecycle
    void Init();
    void Process(const float* const* in, float* const* out, size_t size);
    void Update();

    // Track management
    Track* GetTrack(int index);
    const Track* GetTrack(int index) const;
    void SetActiveTrack(int track);
    int GetActiveTrack() const;
    int GetTrackCount() const;

    // PlayMode management
    void SetPlayMode(std::unique_ptr<IPlayModePlugin> play_mode);
    void ClearPlayMode();
    IPlayModePlugin* GetCurrentPlayMode() const;
    bool IsPlayModeActive() const;

    // Set system references (called from main.cpp)
    void SetVolumeManager(IVolumeManager* volume_mgr) { volume_manager_ = volume_mgr; }
    IVolumeManager* GetVolumeManager() const { return volume_manager_; }
    void SetOctaveShift(OctaveShift* octave_shift);
    OctaveShift* GetOctaveShift() const { return octave_shift_; }

    // UI and control handling
    void UpdateUI();
    void HandleEncoder(int encoder, float delta);
    void HandleButton(int button, bool pressed);
    void HandleJoystick(float x, float y);

    // MIDI handling
    void ProcessMIDI(const MidiEvent* events, size_t count);
    void SendMIDI(const MidiEvent* events, size_t count);

    // System state
    void SetTempo(float bpm);
    float GetTempo() const;
    void SetTimeSignature(int numerator, int denominator);
    void GetTimeSignature(int* numerator, int* denominator) const;

    // Project management
    void SaveProject(const char* filename);
    void LoadProject(const char* filename);
    void NewProject();

    // Audio settings
    void SetSampleRate(float sample_rate);
    float GetSampleRate() const;
    void SetBufferSize(size_t buffer_size);
    size_t GetBufferSize() const;

private:
    // System references (set from main.cpp)
    IVolumeManager* volume_manager_;
    OctaveShift* octave_shift_;
    
    // Tracks
    std::vector<std::unique_ptr<Track>> tracks_;
    int active_track_;

    // PlayMode
    std::unique_ptr<IPlayModePlugin> current_play_mode_;

    // System state
    float tempo_;
    int time_signature_numerator_;
    int time_signature_denominator_;
    float sample_rate_;
    size_t buffer_size_;

    // MIDI processing
    std::vector<MidiEvent> midi_buffer_;
    uint32_t sample_clock_;

    // Internal methods
    void ProcessTracks(const float* const* in, float* const* out, size_t size);
    void UpdateSampleClock();
};

} // namespace OpenChord 