#include "delay_fx.h"
#include <cmath>
#include <cstring>

namespace OpenChord {

DelayFX::DelayFX()
    : sample_rate_(48000.0f)
    , delay_time_(250.0f)  // 250ms default
    , feedback_(0.3f)      // 30% feedback
    , wet_dry_(0.5f)       // 50/50 mix
    , bypassed_(true)      // Start bypassed (off by default)
    , delay_time_setting_value_(250.0f)
    , feedback_setting_value_(0.3f)
    , wet_dry_setting_value_(0.5f)
    , bypassed_setting_value_(true)  // Start bypassed (off by default)
    , initialized_(false)
{
    InitializeSettings();
}

DelayFX::~DelayFX() {
}

void DelayFX::Init() {
    if (sample_rate_ <= 0.0f) {
        sample_rate_ = 48000.0f;
    }
    
    delay_line_.Init();
    UpdateDelayParams();
    
    initialized_ = true;
}

void DelayFX::Process(const float* const* in, float* const* out, size_t size) {
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
        
        // Read delayed sample
        float delayed = delay_line_.Read();
        
        // Mix input with feedback
        float delayed_input = in_sample + delayed * feedback_;
        
        // Write to delay line
        delay_line_.Write(delayed_input);
        
        // Mix wet/dry
        float wet = delayed * wet_dry_;
        float dry = in_sample * (1.0f - wet_dry_);
        float output = wet + dry;
        
        // Output stereo (same signal to both channels)
        out[0][i] = output;
        out[1][i] = output;
    }
}

void DelayFX::Update() {
    UpdateDelayParams();
}

void DelayFX::UpdateUI() {
    // UI handled by settings system
}

void DelayFX::HandleEncoder(int encoder, float delta) {
    (void)encoder;
    (void)delta;
}

void DelayFX::HandleButton(int button, bool pressed) {
    (void)button;
    (void)pressed;
}

void DelayFX::HandleJoystick(float x, float y) {
    (void)x;
    (void)y;
}

void DelayFX::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    if (initialized_) {
        Init();
    }
}

void DelayFX::SetBypass(bool bypass) {
    bypassed_ = bypass;
    bypassed_setting_value_ = bypass;
}

void DelayFX::SetWetDry(float wet_dry) {
    wet_dry_ = wet_dry;
    if (wet_dry_ < 0.0f) wet_dry_ = 0.0f;
    if (wet_dry_ > 1.0f) wet_dry_ = 1.0f;
    wet_dry_setting_value_ = wet_dry_;
}

void DelayFX::InitializeSettings() {
    // Delay Time (float ms)
    settings_[0].name = "Delay Time";
    settings_[0].type = SettingType::FLOAT;
    settings_[0].value_ptr = &delay_time_setting_value_;
    settings_[0].min_value = 0.0f;
    settings_[0].max_value = 1000.0f;
    settings_[0].step_size = 1.0f;
    settings_[0].enum_options = nullptr;
    settings_[0].enum_count = 0;
    settings_[0].on_change_callback = nullptr;
    
    // Feedback (float 0-1)
    settings_[1].name = "Feedback";
    settings_[1].type = SettingType::FLOAT;
    settings_[1].value_ptr = &feedback_setting_value_;
    settings_[1].min_value = 0.0f;
    settings_[1].max_value = 1.0f;
    settings_[1].step_size = 0.01f;
    settings_[1].enum_options = nullptr;
    settings_[1].enum_count = 0;
    settings_[1].on_change_callback = nullptr;
    
    // Wet/Dry (float 0-1)
    settings_[2].name = "Wet/Dry";
    settings_[2].type = SettingType::FLOAT;
    settings_[2].value_ptr = &wet_dry_setting_value_;
    settings_[2].min_value = 0.0f;
    settings_[2].max_value = 1.0f;
    settings_[2].step_size = 0.01f;
    settings_[2].enum_options = nullptr;
    settings_[2].enum_count = 0;
    settings_[2].on_change_callback = nullptr;
    
    // Bypass (bool)
    settings_[3].name = "Bypass";
    settings_[3].type = SettingType::BOOL;
    settings_[3].value_ptr = &bypassed_setting_value_;
    settings_[3].min_value = 0.0f;
    settings_[3].max_value = 1.0f;
    settings_[3].step_size = 1.0f;
    settings_[3].enum_options = nullptr;
    settings_[3].enum_count = 0;
    settings_[3].on_change_callback = nullptr;
}

int DelayFX::GetSettingCount() const {
    return 4;
}

const PluginSetting* DelayFX::GetSetting(int index) const {
    if (index >= 0 && index < GetSettingCount()) {
        return &settings_[index];
    }
    return nullptr;
}

void DelayFX::OnSettingChanged(int setting_index) {
    delay_time_ = delay_time_setting_value_;
    feedback_ = feedback_setting_value_;
    wet_dry_ = wet_dry_setting_value_;
    bypassed_ = bypassed_setting_value_;
    
    UpdateDelayParams();
}

void DelayFX::UpdateDelayParams() {
    if (!initialized_) return;
    
    // Convert delay time from ms to samples (use float for smooth interpolation)
    float delay_samples = (delay_time_ / 1000.0f) * sample_rate_;
    if (delay_samples < 1.0f) delay_samples = 1.0f;
    if (delay_samples > 48000.0f) delay_samples = 48000.0f;
    
    delay_line_.SetDelay(delay_samples);  // DelayLine supports float for interpolation
}

void DelayFX::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    struct State {
        float delay_time;
        float feedback;
        float wet_dry;
        bool bypassed;
    };
    
    State state;
    state.delay_time = delay_time_;
    state.feedback = feedback_;
    state.wet_dry = wet_dry_;
    state.bypassed = bypassed_;
    
    std::memcpy(buffer, &state, sizeof(State));
    *size = sizeof(State);
}

void DelayFX::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(float) * 3 + sizeof(bool)) return;
    
    struct State {
        float delay_time;
        float feedback;
        float wet_dry;
        bool bypassed;
    };
    
    const State* state = reinterpret_cast<const State*>(buffer);
    delay_time_ = state->delay_time;
    feedback_ = state->feedback;
    wet_dry_ = state->wet_dry;
    bypassed_ = state->bypassed;
    
    // Update setting values
    delay_time_setting_value_ = delay_time_;
    feedback_setting_value_ = feedback_;
    wet_dry_setting_value_ = wet_dry_;
    bypassed_setting_value_ = bypassed_;
    
    // Update effect parameters
    if (initialized_) {
        UpdateDelayParams();
    }
}

size_t DelayFX::GetStateSize() const {
    return sizeof(float) * 3 + sizeof(bool);
}

} // namespace OpenChord

