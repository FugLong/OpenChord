#include "audio_engine.h"
#include "../system_interface.h"
#include <cmath>

namespace OpenChord {

AudioEngine::AudioEngine() 
    : hw_(nullptr)
    , volume_manager_(nullptr)
    , system_(nullptr)
    , initialized_(false)
    , input_source_(AudioInputSource::LINE_IN)
    , audio_input_processing_enabled_(false)
{
}

AudioEngine::~AudioEngine() {
}

void AudioEngine::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    initialized_ = true;
}

void AudioEngine::ProcessAudio(const float* const* in, float* const* out, size_t size) {
    if (!initialized_) {
        // Output silence if not ready
        for (size_t i = 0; i < size; i++) {
            out[0][i] = 0.0f;
            out[1][i] = 0.0f;
        }
        return;
    }

    // Process audio input only if processing is enabled and source is selected
    if (audio_input_processing_enabled_) {
        if (input_source_ == AudioInputSource::MICROPHONE) {
            // Microphone passthrough mode (for wiring test)
            // NOTE: Using ADC for audio is not ideal, but we're pin-limited
            // Reading ADC twice per block to get ~24kHz effective rate (instead of 12kHz)
            const VolumeData& volume_data = volume_manager_->GetVolumeData();
            
            // ADC parameters - MAX9814 outputs centered at ~0.38 (1.25V/3.3V)
            static float mic_bias = 0.38f;
            static float mic_scale = 3.0f;
            
            // Read ADC at start of block
            float mic_adc_start = hw_->adc.GetFloat(1);
            float mic_start = (mic_adc_start - mic_bias) * mic_scale;
            
            // Process first half of block
            size_t midpoint = size / 2;
            for (size_t i = 0; i < midpoint; i++) {
                float mic_output = mic_start * volume_data.line_level;
                if (mic_output > 1.0f) mic_output = 1.0f;
                if (mic_output < -1.0f) mic_output = -1.0f;
                out[0][i] = mic_output;
                out[1][i] = mic_output;
            }
            
            // Read ADC again at midpoint for 24kHz effective rate
            float mic_adc_mid = hw_->adc.GetFloat(1);
            float mic_mid = (mic_adc_mid - mic_bias) * mic_scale;
            
            // Linear interpolation for second half
            float mic_step = (mic_mid - mic_start) / static_cast<float>(size - midpoint);
            float mic_value = mic_start + (mic_step * static_cast<float>(midpoint));
            
            // Process second half with interpolation
            for (size_t i = midpoint; i < size; i++) {
                mic_value += mic_step;
                float mic_output = mic_value * volume_data.line_level;
                if (mic_output > 1.0f) mic_output = 1.0f;
                if (mic_output < -1.0f) mic_output = -1.0f;
                out[0][i] = mic_output;
                out[1][i] = mic_output;
            }
            
            return;  // Skip track processing when in mic passthrough mode
        } else if (input_source_ == AudioInputSource::LINE_IN && in != nullptr) {
            // Line input mode - process audio from audio jack (stereo line in)
            const VolumeData& volume_data = volume_manager_->GetVolumeData();
            
            // Process line input with volume control
            for (size_t i = 0; i < size; i++) {
                float line_input = (in[0][i] + in[1][i]) * 0.5f;  // Average stereo to mono
                line_input *= volume_data.line_level;
                
                if (line_input > 1.0f) line_input = 1.0f;
                if (line_input < -1.0f) line_input = -1.0f;
                
                out[0][i] = line_input;
                out[1][i] = line_input;
            }
            
            return;  // Skip track processing when in line input passthrough mode
        }
    }
    
    // Normal mode: Route audio through system (tracks)
    if (system_) {
        system_->Process(in, out, size);
    } else {
        // No system - output silence
        for (size_t i = 0; i < size; i++) {
            out[0][i] = 0.0f;
            out[1][i] = 0.0f;
        }
    }
    
    // Apply volume control to final output
    if (volume_manager_) {
        const VolumeData& volume_data = volume_manager_->GetVolumeData();
        for (size_t i = 0; i < size; i++) {
            out[0][i] *= volume_data.line_level;
            out[1][i] *= volume_data.line_level;
        }
    }
}

void AudioEngine::SetVolumeManager(IVolumeManager* volume_manager) {
    volume_manager_ = volume_manager;
}

void AudioEngine::SetInputSource(AudioInputSource source) {
    input_source_ = source;
}

void AudioEngine::SetAudioInputProcessingEnabled(bool enabled) {
    audio_input_processing_enabled_ = enabled;
}

bool AudioEngine::IsNoteOn() const {
    // Query system for active notes (future enhancement)
    // For now, return false as we don't have a way to query this yet
    return false;
}

} // namespace OpenChord
