#include "audio_engine.h"
#include <cmath>

namespace OpenChord {

AudioEngine::AudioEngine() 
    : hw_(nullptr), volume_manager_(nullptr), initialized_(false),
      current_freq_(440.0f), gate_signal_(false) {
}

AudioEngine::~AudioEngine() {
}

void AudioEngine::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    
    // Initialize test oscillator
    test_osc_.Init(hw->AudioSampleRate());
    test_osc_.SetWaveform(daisysp::Oscillator::WAVE_SAW);
    test_osc_.SetFreq(current_freq_);  // A4 note
    test_osc_.SetAmp(1.0f);     // Full amplitude (volume controlled by volume manager)
    
    // Initialize ADSR envelope with piano-like settings
    envelope_.Init(hw->AudioSampleRate());
    envelope_.SetAttackTime(0.01f);   // 10ms attack (smooth fade-in)
    envelope_.SetDecayTime(0.1f);     // 100ms decay
    envelope_.SetSustainLevel(0.7f);  // 70% sustain
    envelope_.SetReleaseTime(0.1f);   // 100ms release
    
    // Set gate to true so envelope is always active (for constant tone)
    gate_signal_ = true;
    
    initialized_ = true;
}

void AudioEngine::ProcessAudio(const float* const* in, float* const* out, size_t size) {
    if (!initialized_) {
        // Output silence if not ready
        for (size_t i = 0; i < size; i++) {
            out[0][i] = 0.0f;     // Left channel
            out[1][i] = 0.0f;     // Right channel
        }
        return;
    }

    // Get current volume data from volume manager
    const VolumeData& volume_data = volume_manager_->GetVolumeData();
    
    // Debug: Check if we're getting reasonable volume values
    static uint32_t audio_debug_counter = 0;
    if (++audio_debug_counter % 1000 == 0) {
        // We can't safely print from audio callback, but we can verify the values are reasonable
        // If volume_data.amplitude and line_level are both 0, that would explain no audio
    }
    
    // Process the ADSR envelope
    float env_value = envelope_.Process(gate_signal_);
    
    // Apply coordinated volume control (our working method)
    // Set oscillator amplitude for fine low-end control
    test_osc_.SetAmp(volume_data.amplitude);
    
    // Generate audio with volume control and envelope
    for (size_t i = 0; i < size; i++) {
        float sample = test_osc_.Process();
        
        // Apply line level scaling and envelope
        sample *= volume_data.line_level * env_value;
        
        // Output to both channels (stereo)
        out[0][i] = sample;     // Left channel
        out[1][i] = sample;     // Right channel
    }
}

void AudioEngine::SetVolumeManager(IVolumeManager* volume_manager) {
    volume_manager_ = volume_manager;
}

void AudioEngine::SetTestFrequency(float frequency) {
    if (initialized_) {
        current_freq_ = frequency;
        test_osc_.SetFreq(frequency);
    }
}

void AudioEngine::SetTestWaveform(uint8_t waveform) {
    if (initialized_) {
        test_osc_.SetWaveform(waveform);
    }
}

// MIDI note control methods
void AudioEngine::SetFrequency(float freq) {
    if (initialized_) {
        current_freq_ = freq;
        test_osc_.SetFreq(freq);
    }
}

void AudioEngine::NoteOn() {
    if (initialized_) {
        gate_signal_ = true;
        envelope_.Retrigger(false); // Soft retrigger to avoid clicks
    }
}

void AudioEngine::NoteOff() {
    if (initialized_) {
        gate_signal_ = false;
    }
}

bool AudioEngine::IsNoteOn() const {
    return gate_signal_;
}

float AudioEngine::GetCurrentFreq() const {
    return current_freq_;
}

// ADSR envelope adjustment methods
void AudioEngine::SetAttackTime(float attack_ms) {
    if (initialized_) {
        envelope_.SetAttackTime(attack_ms / 1000.0f);
    }
}

void AudioEngine::SetDecayTime(float decay_ms) {
    if (initialized_) {
        envelope_.SetDecayTime(decay_ms / 1000.0f);
    }
}

void AudioEngine::SetSustainLevel(float sustain_percent) {
    if (initialized_) {
        envelope_.SetSustainLevel(sustain_percent / 100.0f);
    }
}

void AudioEngine::SetReleaseTime(float release_ms) {
    if (initialized_) {
        envelope_.SetReleaseTime(release_ms / 1000.0f);
    }
}

} // namespace OpenChord
