#pragma once

#include "daisy_seed.h"
#include "daisysp.h"
#include "volume_interface.h"

namespace OpenChord {

// Forward declaration
class OpenChordSystem;

/**
 * AudioEngine - Audio routing and mixing manager
 * 
 * Routes audio through the OpenChordSystem (tracks) and handles
 * volume management and audio input processing.
 */
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();
    
    // Initialization
    void Init(daisy::DaisySeed* hw);
    
    // Audio processing (called by Daisy audio callback)
    void ProcessAudio(const float* const* in, float* const* out, size_t size);
    
    // Volume control integration
    void SetVolumeManager(IVolumeManager* volume_manager);
    
    // System integration
    void SetSystem(OpenChordSystem* system) { system_ = system; }
    
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
    
    // Query if any audio is playing (for power management)
    bool IsNoteOn() const;
    
private:
    daisy::DaisySeed* hw_;
    IVolumeManager* volume_manager_;
    OpenChordSystem* system_;
    
    // Audio processing state
    bool initialized_;
    AudioInputSource input_source_;  // Selected audio input source
    bool audio_input_processing_enabled_;  // Enable/disable processing of selected source
    
    // Temporary buffers for track mixing
    float track_output_buffer_[2][64];  // Max 64 samples per block
};

} // namespace OpenChord
