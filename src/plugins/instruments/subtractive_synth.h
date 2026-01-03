#pragma once

#include "../../core/plugin_interface.h"
#include "../../core/ui/plugin_settings.h"
#include "daisysp.h"
#include <vector>
#include <cstddef>

namespace OpenChord {

/**
 * SubtractiveSynth - Simple subtractive synthesizer plugin
 * 
 * Features:
 * - 8-voice polyphony
 * - Oscillator with 4 waveforms (saw, square, triangle, sine)
 * - Low-pass filter with cutoff and resonance
 * - ADSR envelope
 * - Concise but powerful settings
 */
class SubtractiveSynth : public IInstrumentPlugin, public IPluginWithSettings {
public:
    SubtractiveSynth();
    ~SubtractiveSynth();
    
    // IPlugin interface
    void Init() override;
    void Process(const float* const* in, float* const* out, size_t size) override;
    void Update() override;
    void UpdateUI() override;
    void HandleEncoder(int encoder, float delta) override;
    void HandleButton(int button, bool pressed) override;
    void HandleJoystick(float x, float y) override;
    
    const char* GetName() const override { return "Subtractive"; }
    const char* GetCategory() const override { return "Instrument"; }
    int GetVersion() const override { return 1; }
    
    void SaveState(void* buffer, size_t* size) const override;
    void LoadState(const void* buffer, size_t size) override;
    size_t GetStateSize() const override;
    
    // IInstrumentPlugin interface
    void NoteOn(int note, float velocity) override;
    void NoteOff(int note) override;
    void AllNotesOff() override;
    void SetPitchBend(float bend) override;
    void SetModulation(float mod) override;
    
    int GetMaxPolyphony() const override { return MAX_VOICES; }
    int GetActiveVoices() const override;
    
    // Settings
    void SetSampleRate(float sample_rate);
    
    // IPluginWithSettings interface
    int GetSettingCount() const override;
    const PluginSetting* GetSetting(int index) const override;
    void OnSettingChanged(int setting_index) override;
    
private:
    static constexpr int MAX_VOICES = 8;
    
    struct Voice {
        daisysp::Oscillator osc;
        daisysp::Svf filter;
        daisysp::Adsr envelope;
        int note;
        float velocity;
        bool active;
        float pitch_bend;
        
        Voice() : note(0), velocity(0.0f), active(false), pitch_bend(0.0f) {}
    };
    
    void InitializeSettings();
    void UpdateOscillatorParams();
    void UpdateFilterParams();
    void UpdateEnvelopeParams();
    Voice* FindFreeVoice();
    Voice* FindVoiceByNote(int note);
    
    // Synthesis parameters
    float sample_rate_;
    int waveform_;  // 0=saw, 1=square, 2=triangle, 3=sine
    float osc_level_;
    float filter_cutoff_;
    float filter_resonance_;
    float envelope_attack_;
    float envelope_decay_;
    float envelope_sustain_;
    float envelope_release_;
    float master_level_;
    
    // Settings storage (for IPluginWithSettings)
    int waveform_setting_value_;
    float osc_level_setting_value_;
    float filter_cutoff_setting_value_;
    float filter_resonance_setting_value_;
    float envelope_attack_setting_value_;
    float envelope_decay_setting_value_;
    float envelope_sustain_setting_value_;
    float envelope_release_setting_value_;
    float master_level_setting_value_;
    
    // Settings array
    PluginSetting settings_[10];
    
    // Voices
    std::vector<Voice> voices_;
    bool initialized_;
    
    // Global modulation
    float pitch_bend_;
    float modulation_;
    
    // Audio buffers (interleaved stereo)
    float audio_buffer_[128 * 2];  // Max 64 samples per block, stereo
};

} // namespace OpenChord

