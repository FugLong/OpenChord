#include "subtractive_synth.h"
#include "daisysp.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace OpenChord {

// MIDI note to frequency conversion
static float mtof(int note) {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

// Waveform names for settings
static const char* waveform_names[] = {
    "Saw", "Square", "Triangle", "Sine", nullptr
};

SubtractiveSynth::SubtractiveSynth()
    : sample_rate_(48000.0f)
    , waveform_(0)  // Saw
    , osc_level_(0.8f)
    , filter_cutoff_(0.5f)
    , filter_resonance_(0.3f)
    , envelope_attack_(10.0f)
    , envelope_decay_(200.0f)
    , envelope_sustain_(0.7f)
    , envelope_release_(300.0f)
    , master_level_(0.8f)
    , waveform_setting_value_(0)
    , osc_level_setting_value_(0.8f)
    , filter_cutoff_setting_value_(0.5f)
    , filter_resonance_setting_value_(0.3f)
    , envelope_attack_setting_value_(10.0f)
    , envelope_decay_setting_value_(200.0f)
    , envelope_sustain_setting_value_(0.7f)
    , envelope_release_setting_value_(300.0f)
    , master_level_setting_value_(0.8f)
    , initialized_(false)
    , pitch_bend_(0.0f)
    , modulation_(0.0f)
{
    voices_.resize(MAX_VOICES);
    InitializeSettings();
}

SubtractiveSynth::~SubtractiveSynth() {
}

void SubtractiveSynth::Init() {
    if (sample_rate_ <= 0.0f) {
        sample_rate_ = 48000.0f;  // Default fallback
    }
    
    // Initialize all voices
    for (auto& voice : voices_) {
        voice.osc.Init(sample_rate_);
        voice.filter.Init(sample_rate_);
        voice.envelope.Init(sample_rate_);
        
        // Map waveform index to DaisySP constants: 0=SAW, 1=SQUARE, 2=TRI, 3=SIN
        uint8_t waveform_val = 0;
        switch(waveform_) {
            case 0: waveform_val = daisysp::Oscillator::WAVE_SAW; break;
            case 1: waveform_val = daisysp::Oscillator::WAVE_SQUARE; break;
            case 2: waveform_val = daisysp::Oscillator::WAVE_TRI; break;
            case 3: waveform_val = daisysp::Oscillator::WAVE_SIN; break;
            default: waveform_val = daisysp::Oscillator::WAVE_SAW; break;
        }
        voice.osc.SetWaveform(waveform_val);
        voice.osc.SetAmp(osc_level_);
        
        voice.filter.SetFreq(filter_cutoff_ * 8000.0f + 100.0f);  // 100-8100 Hz
        voice.filter.SetRes(filter_resonance_);
        voice.filter.SetDrive(0.0f);
        
        voice.envelope.SetAttackTime(envelope_attack_ / 1000.0f);
        voice.envelope.SetDecayTime(envelope_decay_ / 1000.0f);
        voice.envelope.SetSustainLevel(envelope_sustain_);
        voice.envelope.SetReleaseTime(envelope_release_ / 1000.0f);
        
        voice.note = 0;
        voice.velocity = 0.0f;
        voice.active = false;
        voice.pitch_bend = 0.0f;
    }
    
    pitch_bend_ = 0.0f;
    modulation_ = 0.0f;
    
    initialized_ = true;
    UpdateOscillatorParams();
    UpdateFilterParams();
    UpdateEnvelopeParams();
}

void SubtractiveSynth::Process(const float* const* in, float* const* out, size_t size) {
    (void)in;  // Instruments generate from silence, input not used
    
    if (!initialized_) {
        // Output silence
        for (size_t i = 0; i < size; i++) {
            out[0][i] = 0.0f;
            out[1][i] = 0.0f;
        }
        return;
    }
    
    // Process each sample
    for (size_t i = 0; i < size; i++) {
        float sample = 0.0f;
        
        // Process each voice
        for (auto& voice : voices_) {
            if (!voice.active) continue;
            
            // Process envelope
            float env = voice.envelope.Process(voice.active);
            
            // Process oscillator
            float osc_out = voice.osc.Process();
            
            // Process filter (Svf processes input and outputs via getters)
            voice.filter.Process(osc_out);
            float filtered = voice.filter.Low();  // Use low-pass output
            
            // Apply envelope and master level
            sample += filtered * env * master_level_;
        }
        
        // Soft clipping
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        
        // Write to output (deinterleaved stereo)
        out[0][i] = sample;  // Left
        out[1][i] = sample;  // Right
    }
}

void SubtractiveSynth::Update() {
    // Update parameters if settings changed
    UpdateOscillatorParams();
    UpdateFilterParams();
    UpdateEnvelopeParams();
}

void SubtractiveSynth::UpdateUI() {
    // UI updates handled by settings system
}

void SubtractiveSynth::HandleEncoder(int encoder, float delta) {
    // Encoder handling via settings system
    (void)encoder;
    (void)delta;
}

void SubtractiveSynth::HandleButton(int button, bool pressed) {
    // Button handling not used
    (void)button;
    (void)pressed;
}

void SubtractiveSynth::HandleJoystick(float x, float y) {
    // Joystick could be used for real-time modulation
    // For now, not used
    (void)x;
    (void)y;
}

void SubtractiveSynth::NoteOn(int note, float velocity) {
    if (!initialized_) return;
    
    // Find free voice
    Voice* voice = FindFreeVoice();
    if (!voice) {
        // All voices busy - steal oldest voice (simple round-robin)
        voice = &voices_[0];
        for (auto& v : voices_) {
            if (v.note < voice->note) {
                voice = &v;
            }
        }
    }
    
    // Set up voice
    voice->note = note;
    voice->velocity = velocity;
    voice->active = true;
    voice->pitch_bend = pitch_bend_;
    
    // Calculate frequency with pitch bend
    float freq = mtof(note) * powf(2.0f, pitch_bend_ / 12.0f);
    voice->osc.SetFreq(freq);
    voice->osc.SetAmp(osc_level_ * velocity);
    
    // Retrigger envelope
    voice->envelope.Retrigger(false);
}

void SubtractiveSynth::NoteOff(int note) {
    if (!initialized_) return;
    
    // Find voice playing this note
    Voice* voice = FindVoiceByNote(note);
    if (voice) {
        voice->active = false;
    }
}

void SubtractiveSynth::AllNotesOff() {
    for (auto& voice : voices_) {
        voice.active = false;
    }
}

void SubtractiveSynth::SetPitchBend(float bend) {
    pitch_bend_ = bend;  // bend is in semitones
    
    // Update all active voices
    for (auto& voice : voices_) {
        if (voice.active) {
            voice.pitch_bend = pitch_bend_;
            float freq = mtof(voice.note) * powf(2.0f, pitch_bend_ / 12.0f);
            voice.osc.SetFreq(freq);
        }
    }
}

void SubtractiveSynth::SetModulation(float mod) {
    modulation_ = mod;
    // Could use modulation for filter cutoff, vibrato, etc.
    // For now, just store it
}

int SubtractiveSynth::GetActiveVoices() const {
    int count = 0;
    for (const auto& voice : voices_) {
        if (voice.active) count++;
    }
    return count;
}

void SubtractiveSynth::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    if (initialized_) {
        // Reinitialize with new sample rate
        Init();
    }
}

SubtractiveSynth::Voice* SubtractiveSynth::FindFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.active) {
            return &voice;
        }
    }
    return nullptr;
}

SubtractiveSynth::Voice* SubtractiveSynth::FindVoiceByNote(int note) {
    for (auto& voice : voices_) {
        if (voice.active && voice.note == note) {
            return &voice;
        }
    }
    return nullptr;
}

void SubtractiveSynth::UpdateOscillatorParams() {
    if (!initialized_) return;
    
    // Map waveform index to DaisySP constants
    uint8_t waveform_val = 0;
    switch(waveform_) {
        case 0: waveform_val = daisysp::Oscillator::WAVE_SAW; break;
        case 1: waveform_val = daisysp::Oscillator::WAVE_SQUARE; break;
        case 2: waveform_val = daisysp::Oscillator::WAVE_TRI; break;
        case 3: waveform_val = daisysp::Oscillator::WAVE_SIN; break;
        default: waveform_val = daisysp::Oscillator::WAVE_SAW; break;
    }
    
    for (auto& voice : voices_) {
        voice.osc.SetWaveform(waveform_val);
        if (voice.active) {
            voice.osc.SetAmp(osc_level_ * voice.velocity);
        }
    }
}

void SubtractiveSynth::UpdateFilterParams() {
    if (!initialized_) return;
    
    float cutoff_hz = filter_cutoff_ * 8000.0f + 100.0f;  // 100-8100 Hz
    
    for (auto& voice : voices_) {
        voice.filter.SetFreq(cutoff_hz);
        voice.filter.SetRes(filter_resonance_);
    }
}

void SubtractiveSynth::UpdateEnvelopeParams() {
    if (!initialized_) return;
    
    for (auto& voice : voices_) {
        voice.envelope.SetAttackTime(envelope_attack_ / 1000.0f);
        voice.envelope.SetDecayTime(envelope_decay_ / 1000.0f);
        voice.envelope.SetSustainLevel(envelope_sustain_);
        voice.envelope.SetReleaseTime(envelope_release_ / 1000.0f);
    }
}

void SubtractiveSynth::InitializeSettings() {
    // Waveform (enum)
    settings_[0].name = "Waveform";
    settings_[0].type = SettingType::ENUM;
    settings_[0].value_ptr = &waveform_setting_value_;
    settings_[0].min_value = 0.0f;
    settings_[0].max_value = 3.0f;
    settings_[0].step_size = 1.0f;
    settings_[0].enum_options = waveform_names;
    settings_[0].enum_count = 4;
    settings_[0].on_change_callback = nullptr;
    
    // Osc Level (float 0-1)
    settings_[1].name = "Osc Level";
    settings_[1].type = SettingType::FLOAT;
    settings_[1].value_ptr = &osc_level_setting_value_;
    settings_[1].min_value = 0.0f;
    settings_[1].max_value = 1.0f;
    settings_[1].step_size = 0.01f;
    settings_[1].enum_options = nullptr;
    settings_[1].enum_count = 0;
    settings_[1].on_change_callback = nullptr;
    
    // Filter Cutoff (float 0-1)
    settings_[2].name = "Filter Cutoff";
    settings_[2].type = SettingType::FLOAT;
    settings_[2].value_ptr = &filter_cutoff_setting_value_;
    settings_[2].min_value = 0.0f;
    settings_[2].max_value = 1.0f;
    settings_[2].step_size = 0.01f;
    settings_[2].enum_options = nullptr;
    settings_[2].enum_count = 0;
    settings_[2].on_change_callback = nullptr;
    
    // Filter Resonance (float 0-1)
    settings_[3].name = "Resonance";
    settings_[3].type = SettingType::FLOAT;
    settings_[3].value_ptr = &filter_resonance_setting_value_;
    settings_[3].min_value = 0.0f;
    settings_[3].max_value = 1.0f;
    settings_[3].step_size = 0.01f;
    settings_[3].enum_options = nullptr;
    settings_[3].enum_count = 0;
    settings_[3].on_change_callback = nullptr;
    
    // Envelope Attack (float ms)
    settings_[4].name = "Attack";
    settings_[4].type = SettingType::FLOAT;
    settings_[4].value_ptr = &envelope_attack_setting_value_;
    settings_[4].min_value = 1.0f;
    settings_[4].max_value = 5000.0f;
    settings_[4].step_size = 1.0f;
    settings_[4].enum_options = nullptr;
    settings_[4].enum_count = 0;
    settings_[4].on_change_callback = nullptr;
    
    // Envelope Decay (float ms)
    settings_[5].name = "Decay";
    settings_[5].type = SettingType::FLOAT;
    settings_[5].value_ptr = &envelope_decay_setting_value_;
    settings_[5].min_value = 1.0f;
    settings_[5].max_value = 5000.0f;
    settings_[5].step_size = 1.0f;
    settings_[5].enum_options = nullptr;
    settings_[5].enum_count = 0;
    settings_[5].on_change_callback = nullptr;
    
    // Envelope Sustain (float 0-1)
    settings_[6].name = "Sustain";
    settings_[6].type = SettingType::FLOAT;
    settings_[6].value_ptr = &envelope_sustain_setting_value_;
    settings_[6].min_value = 0.0f;
    settings_[6].max_value = 1.0f;
    settings_[6].step_size = 0.01f;
    settings_[6].enum_options = nullptr;
    settings_[6].enum_count = 0;
    settings_[6].on_change_callback = nullptr;
    
    // Envelope Release (float ms)
    settings_[7].name = "Release";
    settings_[7].type = SettingType::FLOAT;
    settings_[7].value_ptr = &envelope_release_setting_value_;
    settings_[7].min_value = 1.0f;
    settings_[7].max_value = 5000.0f;
    settings_[7].step_size = 1.0f;
    settings_[7].enum_options = nullptr;
    settings_[7].enum_count = 0;
    settings_[7].on_change_callback = nullptr;
    
    // Master Level (float 0-1)
    settings_[8].name = "Level";
    settings_[8].type = SettingType::FLOAT;
    settings_[8].value_ptr = &master_level_setting_value_;
    settings_[8].min_value = 0.0f;
    settings_[8].max_value = 1.0f;
    settings_[8].step_size = 0.01f;
    settings_[8].enum_options = nullptr;
    settings_[8].enum_count = 0;
    settings_[8].on_change_callback = nullptr;
}

int SubtractiveSynth::GetSettingCount() const {
    return 9;
}

const PluginSetting* SubtractiveSynth::GetSetting(int index) const {
    if (index >= 0 && index < GetSettingCount()) {
        return &settings_[index];
    }
    return nullptr;
}

void SubtractiveSynth::OnSettingChanged(int setting_index) {
    // Update actual parameter values from setting values
    waveform_ = waveform_setting_value_;
    osc_level_ = osc_level_setting_value_;
    filter_cutoff_ = filter_cutoff_setting_value_;
    filter_resonance_ = filter_resonance_setting_value_;
    envelope_attack_ = envelope_attack_setting_value_;
    envelope_decay_ = envelope_decay_setting_value_;
    envelope_sustain_ = envelope_sustain_setting_value_;
    envelope_release_ = envelope_release_setting_value_;
    master_level_ = master_level_setting_value_;
    
    // Update synthesis parameters
    UpdateOscillatorParams();
    UpdateFilterParams();
    UpdateEnvelopeParams();
}

void SubtractiveSynth::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    struct State {
        int waveform;
        float osc_level;
        float filter_cutoff;
        float filter_resonance;
        float envelope_attack;
        float envelope_decay;
        float envelope_sustain;
        float envelope_release;
        float master_level;
    };
    
    State state;
    state.waveform = waveform_;
    state.osc_level = osc_level_;
    state.filter_cutoff = filter_cutoff_;
    state.filter_resonance = filter_resonance_;
    state.envelope_attack = envelope_attack_;
    state.envelope_decay = envelope_decay_;
    state.envelope_sustain = envelope_sustain_;
    state.envelope_release = envelope_release_;
    state.master_level = master_level_;
    
    std::memcpy(buffer, &state, sizeof(State));
    *size = sizeof(State);
}

void SubtractiveSynth::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(int) + sizeof(float) * 8) return;
    
    struct State {
        int waveform;
        float osc_level;
        float filter_cutoff;
        float filter_resonance;
        float envelope_attack;
        float envelope_decay;
        float envelope_sustain;
        float envelope_release;
        float master_level;
    };
    
    const State* state = reinterpret_cast<const State*>(buffer);
    waveform_ = state->waveform;
    osc_level_ = state->osc_level;
    filter_cutoff_ = state->filter_cutoff;
    filter_resonance_ = state->filter_resonance;
    envelope_attack_ = state->envelope_attack;
    envelope_decay_ = state->envelope_decay;
    envelope_sustain_ = state->envelope_sustain;
    envelope_release_ = state->envelope_release;
    master_level_ = state->master_level;
    
    // Update setting values
    waveform_setting_value_ = waveform_;
    osc_level_setting_value_ = osc_level_;
    filter_cutoff_setting_value_ = filter_cutoff_;
    filter_resonance_setting_value_ = filter_resonance_;
    envelope_attack_setting_value_ = envelope_attack_;
    envelope_decay_setting_value_ = envelope_decay_;
    envelope_sustain_setting_value_ = envelope_sustain_;
    envelope_release_setting_value_ = envelope_release_;
    master_level_setting_value_ = master_level_;
    
    // Update synthesis parameters
    if (initialized_) {
        UpdateOscillatorParams();
        UpdateFilterParams();
        UpdateEnvelopeParams();
    }
}

size_t SubtractiveSynth::GetStateSize() const {
    return sizeof(int) + sizeof(float) * 8;
}

} // namespace OpenChord

