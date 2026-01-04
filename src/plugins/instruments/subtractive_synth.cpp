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
    , osc_level_(1.0f)  // Normalized - scaling happens in processing
    , filter_cutoff_(0.5f)
    , filter_resonance_(0.3f)
    , envelope_attack_(10.0f)
    , envelope_decay_(200.0f)
    , envelope_sustain_(0.7f)
    , envelope_release_(300.0f)
    , master_level_(0.8f)
    , waveform_setting_value_(0)
    , osc_level_setting_value_(1.0f)
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
        voice.osc.SetAmp(1.0f);  // Normalized amplitude - scaling done in processing
        
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
            // Only process voices that have been assigned a note
            if (voice.note == 0) continue;
            
            // Process envelope (gate = true when note is actively held)
            float env = voice.envelope.Process(voice.active);
            
            // If envelope is idle (release phase complete), clean up the voice
            if (!voice.envelope.IsRunning()) {
                voice.note = 0;
                voice.velocity = 0.0f;
                voice.active = false;
                continue;
            }
            
            // Clamp envelope to prevent distortion (should be 0-1, but be safe)
            if (env > 1.0f) env = 1.0f;
            if (env < 0.0f) env = 0.0f;
            
            // Process oscillator (amplitude should be normalized, scaling done in processing)
            float osc_out = voice.osc.Process();
            
            // Process filter (Svf processes input and outputs via getters)
            voice.filter.Process(osc_out);
            float filtered = voice.filter.Low();  // Use low-pass output
            
            // Apply oscillator level, envelope, velocity, and master level
            sample += filtered * osc_level_ * env * voice.velocity * master_level_;
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
    
    // Check if this note is already playing - if so, retrigger that voice
    Voice* voice = FindVoiceByNote(note);
    if (voice) {
        // Note already playing - retrigger the same voice
        voice->velocity = velocity;
        voice->active = true;
        voice->pitch_bend = pitch_bend_;
        
        // Calculate frequency with pitch bend
        float freq = mtof(note) * powf(2.0f, pitch_bend_ / 12.0f);
        voice->osc.SetFreq(freq);
        voice->osc.SetAmp(1.0f);  // Normalized amplitude - scaling done in processing
        
        // Ensure envelope parameters are up to date before retriggering
        voice->envelope.SetAttackTime(envelope_attack_ / 1000.0f);
        voice->envelope.SetDecayTime(envelope_decay_ / 1000.0f);
        voice->envelope.SetSustainLevel(envelope_sustain_);
        voice->envelope.SetReleaseTime(envelope_release_ / 1000.0f);
        
        // Retrigger envelope (hard = true resets history, false keeps history - use true for clean attack)
        voice->envelope.Retrigger(true);
        return;
    }
    
    // Find free voice
    voice = FindFreeVoice();
    if (!voice) {
        // All voices busy - steal oldest voice (first voice index, simple but effective)
        // Find the voice with the lowest index that's not actively being held (priority for release)
        voice = &voices_[0];
        for (size_t i = 0; i < voices_.size(); i++) {
            if (!voices_[i].active) {
                voice = &voices_[i];
                break;
            }
        }
        // If all are active, just use the first one
    }
    
    // Reset the voice to clean state before reuse
    ResetVoice(voice);
    
    // Set up voice
    voice->note = note;
    voice->velocity = velocity;
    voice->active = true;
    voice->pitch_bend = pitch_bend_;
    
    // Calculate frequency with pitch bend
    float freq = mtof(note) * powf(2.0f, pitch_bend_ / 12.0f);
    voice->osc.SetFreq(freq);
    voice->osc.SetAmp(1.0f);  // Normalized amplitude - scaling done in processing
    
    // Ensure envelope parameters are up to date before retriggering
    voice->envelope.SetAttackTime(envelope_attack_ / 1000.0f);
    voice->envelope.SetDecayTime(envelope_decay_ / 1000.0f);
    voice->envelope.SetSustainLevel(envelope_sustain_);
    voice->envelope.SetReleaseTime(envelope_release_ / 1000.0f);
    
    // Retrigger envelope (hard = true resets history, false keeps history - use true for clean attack)
    voice->envelope.Retrigger(true);
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
    // Find a voice that's not assigned (note == 0) or has envelope in idle state
    for (auto& voice : voices_) {
        if (voice.note == 0) {
            return &voice;
        }
        if (!voice.envelope.IsRunning() && !voice.active) {
            return &voice;
        }
    }
    return nullptr;
}

void SubtractiveSynth::ResetVoice(Voice* voice) {
    if (!voice || !initialized_) return;
    
    // Reinitialize filter, oscillator, and envelope to reset their internal state
    // This prevents clicks/distortion when reusing voices
    voice->filter.Init(sample_rate_);
    voice->osc.Init(sample_rate_);
    voice->envelope.Init(sample_rate_);
    
    // Reset filter parameters
    float cutoff_hz = filter_cutoff_ * 8000.0f + 100.0f;
    voice->filter.SetFreq(cutoff_hz);
    voice->filter.SetRes(filter_resonance_);
    voice->filter.SetDrive(0.0f);
    
    // Settle filter state by processing a few zero samples
    // This prevents crackling from uninitialized filter state
    for (int i = 0; i < 8; i++) {
        voice->filter.Process(0.0f);
    }
    
    // Reset oscillator parameters
    uint8_t waveform_val = 0;
    switch(waveform_) {
        case 0: waveform_val = daisysp::Oscillator::WAVE_SAW; break;
        case 1: waveform_val = daisysp::Oscillator::WAVE_SQUARE; break;
        case 2: waveform_val = daisysp::Oscillator::WAVE_TRI; break;
        case 3: waveform_val = daisysp::Oscillator::WAVE_SIN; break;
        default: waveform_val = daisysp::Oscillator::WAVE_SAW; break;
    }
    voice->osc.SetWaveform(waveform_val);
    voice->osc.SetAmp(1.0f);  // Normalized amplitude - scaling done in processing
    
    // Reset envelope parameters (will be set properly in NoteOn before Retrigger)
    voice->envelope.SetAttackTime(envelope_attack_ / 1000.0f);
    voice->envelope.SetDecayTime(envelope_decay_ / 1000.0f);
    voice->envelope.SetSustainLevel(envelope_sustain_);
    voice->envelope.SetReleaseTime(envelope_release_ / 1000.0f);
    
    // Clean up voice state
    voice->note = 0;
    voice->velocity = 0.0f;
    voice->active = false;
    voice->pitch_bend = 0.0f;
}

SubtractiveSynth::Voice* SubtractiveSynth::FindVoiceByNote(int note) {
    // First try to find an active voice playing this note
    for (auto& voice : voices_) {
        if (voice.active && voice.note == note) {
            return &voice;
        }
    }
    // Also check voices in release phase (envelope still running)
    for (auto& voice : voices_) {
        if (voice.note == note && voice.envelope.IsRunning()) {
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
        voice.osc.SetAmp(1.0f);  // Normalized amplitude - scaling done in processing
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
    
    // Update envelope parameters for all voices
    // DaisySP Adsr applies these parameters immediately
    // Note: Parameters may only affect future phases of active envelopes,
    // but this is the correct behavior to avoid audio glitches
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
    // Only update the parameter that changed (for efficiency)
    switch (setting_index) {
        case 0: waveform_ = waveform_setting_value_; UpdateOscillatorParams(); break;
        case 1: osc_level_ = osc_level_setting_value_; UpdateOscillatorParams(); break;
        case 2: filter_cutoff_ = filter_cutoff_setting_value_; UpdateFilterParams(); break;
        case 3: filter_resonance_ = filter_resonance_setting_value_; UpdateFilterParams(); break;
        case 4: envelope_attack_ = envelope_attack_setting_value_; UpdateEnvelopeParams(); break;
        case 5: envelope_decay_ = envelope_decay_setting_value_; UpdateEnvelopeParams(); break;
        case 6: envelope_sustain_ = envelope_sustain_setting_value_; UpdateEnvelopeParams(); break;
        case 7: envelope_release_ = envelope_release_setting_value_; UpdateEnvelopeParams(); break;
        case 8: master_level_ = master_level_setting_value_; break;
        default:
            // Fallback: update all parameters (for safety)
            waveform_ = waveform_setting_value_;
            osc_level_ = osc_level_setting_value_;
            filter_cutoff_ = filter_cutoff_setting_value_;
            filter_resonance_ = filter_resonance_setting_value_;
            envelope_attack_ = envelope_attack_setting_value_;
            envelope_decay_ = envelope_decay_setting_value_;
            envelope_sustain_ = envelope_sustain_setting_value_;
            envelope_release_ = envelope_release_setting_value_;
            master_level_ = master_level_setting_value_;
            UpdateOscillatorParams();
            UpdateFilterParams();
            UpdateEnvelopeParams();
            break;
    }
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

