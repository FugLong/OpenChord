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
    
    // Audio input source control
    enum class AudioInputSource {
        LINE_IN,      // Audio jack input (stereo line in)
        MICROPHONE    // Microphone via ADC
    };
    
    // Audio input source selection (selects which source to use)
    void SetInputSource(AudioInputSource source);
    AudioInputSource GetInputSource() const { return input_source_; }
    
    // Audio input processing toggle (enable/disable processing of selected source)
    // When disabled, no audio input is processed (power savings)
    // When enabled, only the selected source is processed
    void SetAudioInputProcessingEnabled(bool enabled);
    bool IsAudioInputProcessingEnabled() const { return audio_input_processing_enabled_; }
    
    // Legacy method for backward compatibility
    void SetMicPassthroughEnabled(bool enabled) { 
        SetInputSource(enabled ? AudioInputSource::MICROPHONE : AudioInputSource::LINE_IN);
        SetAudioInputProcessingEnabled(enabled);
    }
    bool IsMicPassthroughEnabled() const { 
        return input_source_ == AudioInputSource::MICROPHONE && audio_input_processing_enabled_;
    }
    
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
    AudioInputSource input_source_;  // Selected audio input source
    bool audio_input_processing_enabled_;  // Enable/disable processing of selected source
};

} // namespace OpenChord
