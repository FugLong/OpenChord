#pragma once

#include "daisy_seed.h"
#include "daisysp.h"
#include "volume_interface.h"
#include "../midi/midi_interface.h"

namespace OpenChord {

/**
 * Simple audio engine for testing volume control
 * TODO: This will become a full-featured audio engine with:
 * - Multiple oscillators and synthesis engines
 * - Effects processing
 * - Audio routing and mixing
 * - Plugin system integration
 */
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();
    
    // Initialization
    void Init(daisy::DaisySeed* hw);
    
    // Audio processing (called by system)
    void ProcessAudio(const float* const* in, float* const* out, size_t size);
    
    // MIDI processing (called by system)
    void ProcessMidi();
    
    // Volume control integration
    void SetVolumeManager(IVolumeManager* volume_manager);
    
    // Test oscillator control
    void SetTestFrequency(float frequency);
    void SetTestWaveform(uint8_t waveform);
    
    // MIDI note control
    void SetFrequency(float freq);
    void NoteOn();
    void NoteOff();
    bool IsNoteOn() const;
    float GetCurrentFreq() const;
    
    // ADSR envelope adjustment methods
    void SetAttackTime(float attack_ms);
    void SetDecayTime(float decay_ms);
    void SetSustainLevel(float sustain_percent);
    void SetReleaseTime(float release_ms);
    
    // Microphone passthrough (temporary for wiring test)
    void SetMicPassthroughEnabled(bool enabled);
    bool IsMicPassthroughEnabled() const;
    
private:
    // MIDI note to frequency conversion
    float mtof(uint8_t note) const;
    daisy::DaisySeed* hw_;
    IVolumeManager* volume_manager_;
    
    // Test oscillator with Daisy ADSR envelope
    daisysp::Oscillator test_osc_;
    daisysp::Adsr envelope_;
    float current_freq_;
    bool gate_signal_;
    
    // Audio processing state
    bool initialized_;
    bool mic_passthrough_enabled_;
};

} // namespace OpenChord
