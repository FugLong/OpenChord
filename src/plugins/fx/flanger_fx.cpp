#include "flanger_fx.h"
#include <cmath>
#include <cstring>

namespace OpenChord {

FlangerFX::FlangerFX()
    : sample_rate_(48000.0f)
    , lfo_depth_(0.7f)      // Default 70% depth (pronounced effect)
    , lfo_freq_(0.5f)       // Default 0.5 Hz (classic flanger speed)
    , delay_ms_(2.0f)       // Default 2ms delay (typical flanger range)
    , feedback_(0.5f)       // Default 50% feedback (stronger flanger)
    , wet_dry_(0.5f)        // Default 50/50 mix
    , bypassed_(true)       // Start bypassed (off by default)
    , lfo_depth_setting_value_(0.7f)
    , lfo_freq_setting_value_(0.5f)
    , delay_ms_setting_value_(2.0f)
    , feedback_setting_value_(0.5f)
    , wet_dry_setting_value_(0.5f)
    , bypassed_setting_value_(true)  // Start bypassed (off by default)
    , initialized_(false)
{
    InitializeSettings();
}

FlangerFX::~FlangerFX() {
}

void FlangerFX::Init() {
    if (sample_rate_ <= 0.0f) {
        sample_rate_ = 48000.0f;
    }
    
    flanger_.Init(sample_rate_);
    UpdateFlangerParams();
    
    initialized_ = true;
}

void FlangerFX::Process(const float* const* in, float* const* out, size_t size) {
    if (!initialized_ || bypassed_) {
        // Bypass: copy input to output
        for (size_t i = 0; i < size; i++) {
            out[0][i] = in[0][i];
            out[1][i] = in[1][i];
        }
        return;
    }
    
    // Process deinterleaved stereo
    for (size_t i = 0; i < size; i++) {
        // Get input sample (mono from stereo)
        float in_sample = (in[0][i] + in[1][i]) * 0.5f;
        
        // Process through flanger
        float processed = flanger_.Process(in_sample);
        
        // Mix wet/dry
        float wet = processed * wet_dry_;
        float dry = in_sample * (1.0f - wet_dry_);
        float output = wet + dry;
        
        // Output stereo (same signal to both channels)
        out[0][i] = output;
        out[1][i] = output;
    }
}

void FlangerFX::Update() {
    UpdateFlangerParams();
}

void FlangerFX::UpdateUI() {
    // UI handled by settings system
}

void FlangerFX::HandleEncoder(int encoder, float delta) {
    (void)encoder;
    (void)delta;
}

void FlangerFX::HandleButton(int button, bool pressed) {
    (void)button;
    (void)pressed;
}

void FlangerFX::HandleJoystick(float x, float y) {
    (void)x;
    (void)y;
}

void FlangerFX::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    if (initialized_) {
        Init();
    }
}

void FlangerFX::SetBypass(bool bypass) {
    bypassed_ = bypass;
    bypassed_setting_value_ = bypass;
    
    // Initialize when enabled (if not already initialized)
    if (!bypass && !initialized_) {
        Init();
    }
}

void FlangerFX::SetWetDry(float wet_dry) {
    wet_dry_ = wet_dry;
    if (wet_dry_ < 0.0f) wet_dry_ = 0.0f;
    if (wet_dry_ > 1.0f) wet_dry_ = 1.0f;
    wet_dry_setting_value_ = wet_dry_;
}

void FlangerFX::InitializeSettings() {
    // LFO Depth (float 0-1)
    settings_[0].name = "LFO Depth";
    settings_[0].type = SettingType::FLOAT;
    settings_[0].value_ptr = &lfo_depth_setting_value_;
    settings_[0].min_value = 0.0f;
    settings_[0].max_value = 1.0f;
    settings_[0].step_size = 0.01f;
    settings_[0].enum_options = nullptr;
    settings_[0].enum_count = 0;
    settings_[0].on_change_callback = nullptr;
    
    // LFO Frequency (float 0.1-10 Hz)
    settings_[1].name = "LFO Freq";
    settings_[1].type = SettingType::FLOAT;
    settings_[1].value_ptr = &lfo_freq_setting_value_;
    settings_[1].min_value = 0.1f;
    settings_[1].max_value = 10.0f;
    settings_[1].step_size = 0.1f;
    settings_[1].enum_options = nullptr;
    settings_[1].enum_count = 0;
    settings_[1].on_change_callback = nullptr;
    
    // Delay (float 0.1-7ms)
    settings_[2].name = "Delay";
    settings_[2].type = SettingType::FLOAT;
    settings_[2].value_ptr = &delay_ms_setting_value_;
    settings_[2].min_value = 0.1f;
    settings_[2].max_value = 7.0f;
    settings_[2].step_size = 0.1f;
    settings_[2].enum_options = nullptr;
    settings_[2].enum_count = 0;
    settings_[2].on_change_callback = nullptr;
    
    // Feedback (float 0-1)
    settings_[3].name = "Feedback";
    settings_[3].type = SettingType::FLOAT;
    settings_[3].value_ptr = &feedback_setting_value_;
    settings_[3].min_value = 0.0f;
    settings_[3].max_value = 1.0f;
    settings_[3].step_size = 0.01f;
    settings_[3].enum_options = nullptr;
    settings_[3].enum_count = 0;
    settings_[3].on_change_callback = nullptr;
    
    // Wet/Dry (float 0-1)
    settings_[4].name = "Wet/Dry";
    settings_[4].type = SettingType::FLOAT;
    settings_[4].value_ptr = &wet_dry_setting_value_;
    settings_[4].min_value = 0.0f;
    settings_[4].max_value = 1.0f;
    settings_[4].step_size = 0.01f;
    settings_[4].enum_options = nullptr;
    settings_[4].enum_count = 0;
    settings_[4].on_change_callback = nullptr;
    
    // Bypass (bool)
    settings_[5].name = "Bypass";
    settings_[5].type = SettingType::BOOL;
    settings_[5].value_ptr = &bypassed_setting_value_;
    settings_[5].min_value = 0.0f;
    settings_[5].max_value = 1.0f;
    settings_[5].step_size = 1.0f;
    settings_[5].enum_options = nullptr;
    settings_[5].enum_count = 0;
    settings_[5].on_change_callback = nullptr;
}

int FlangerFX::GetSettingCount() const {
    return 6;
}

const PluginSetting* FlangerFX::GetSetting(int index) const {
    if (index >= 0 && index < GetSettingCount()) {
        return &settings_[index];
    }
    return nullptr;
}

void FlangerFX::OnSettingChanged(int setting_index) {
    lfo_depth_ = lfo_depth_setting_value_;
    lfo_freq_ = lfo_freq_setting_value_;
    delay_ms_ = delay_ms_setting_value_;
    feedback_ = feedback_setting_value_;
    wet_dry_ = wet_dry_setting_value_;
    bypassed_ = bypassed_setting_value_;
    
    UpdateFlangerParams();
}

void FlangerFX::UpdateFlangerParams() {
    if (!initialized_) return;
    
    flanger_.SetLfoDepth(lfo_depth_);
    flanger_.SetLfoFreq(lfo_freq_);
    flanger_.SetDelayMs(delay_ms_);
    flanger_.SetFeedback(feedback_);
}

void FlangerFX::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    struct State {
        float lfo_depth;
        float lfo_freq;
        float delay_ms;
        float feedback;
        float wet_dry;
        bool bypassed;
    };
    
    State state;
    state.lfo_depth = lfo_depth_;
    state.lfo_freq = lfo_freq_;
    state.delay_ms = delay_ms_;
    state.feedback = feedback_;
    state.wet_dry = wet_dry_;
    state.bypassed = bypassed_;
    
    std::memcpy(buffer, &state, sizeof(State));
    *size = sizeof(State);
}

void FlangerFX::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(float) * 5 + sizeof(bool)) return;
    
    struct State {
        float lfo_depth;
        float lfo_freq;
        float delay_ms;
        float feedback;
        float wet_dry;
        bool bypassed;
    };
    
    const State* state = reinterpret_cast<const State*>(buffer);
    lfo_depth_ = state->lfo_depth;
    lfo_freq_ = state->lfo_freq;
    delay_ms_ = state->delay_ms;
    feedback_ = state->feedback;
    wet_dry_ = state->wet_dry;
    bypassed_ = state->bypassed;
    
    // Update setting values
    lfo_depth_setting_value_ = lfo_depth_;
    lfo_freq_setting_value_ = lfo_freq_;
    delay_ms_setting_value_ = delay_ms_;
    feedback_setting_value_ = feedback_;
    wet_dry_setting_value_ = wet_dry_;
    bypassed_setting_value_ = bypassed_;
    
    // Update effect parameters
    if (initialized_) {
        UpdateFlangerParams();
    }
}

size_t FlangerFX::GetStateSize() const {
    return sizeof(float) * 5 + sizeof(bool);
}

} // namespace OpenChord

