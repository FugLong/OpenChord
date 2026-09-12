#include "phaser_fx.h"
#include <cmath>
#include <cstring>

namespace OpenChord {

PhaserFX::PhaserFX()
    : sample_rate_(48000.0f)
    , lfo_depth_(0.7f)       // Default 70% depth
    , lfo_freq_(0.5f)        // Default 0.5 Hz
    , ap_freq_(1000.0f)      // Default 1kHz allpass
    , feedback_(0.3f)        // Default 30% feedback
    , poles_(4)              // Default 4 poles
    , wet_dry_(0.5f)         // Default 50/50 mix
    , bypassed_(true)        // Start bypassed (off by default)
    , lfo_depth_setting_value_(0.7f)
    , lfo_freq_setting_value_(0.5f)
    , ap_freq_setting_value_(1000.0f)
    , feedback_setting_value_(0.3f)
    , poles_setting_value_(4.0f)
    , wet_dry_setting_value_(0.5f)
    , bypassed_setting_value_(true)  // Start bypassed (off by default)
    , initialized_(false)
{
    InitializeSettings();
}

PhaserFX::~PhaserFX() {
}

void PhaserFX::Init() {
    if (sample_rate_ <= 0.0f) {
        sample_rate_ = 48000.0f;
    }
    
    phaser_.Init(sample_rate_);
    initialized_ = true;  // Set before UpdatePhaserParams so it can run
    UpdatePhaserParams();
}

void PhaserFX::Process(const float* const* in, float* const* out, size_t size) {
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
        
        // Process through phaser
        float processed = phaser_.Process(in_sample);
        
        // Mix wet/dry
        float wet = processed * wet_dry_;
        float dry = in_sample * (1.0f - wet_dry_);
        float output = wet + dry;
        
        // Output stereo (same signal to both channels)
        out[0][i] = output;
        out[1][i] = output;
    }
}

void PhaserFX::Update() {
    UpdatePhaserParams();
}

void PhaserFX::UpdateUI() {
    // UI handled by settings system
}

void PhaserFX::HandleEncoder(int encoder, float delta) {
    (void)encoder;
    (void)delta;
}

void PhaserFX::HandleButton(int button, bool pressed) {
    (void)button;
    (void)pressed;
}

void PhaserFX::HandleJoystick(float x, float y) {
    (void)x;
    (void)y;
}

void PhaserFX::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    if (initialized_) {
        Init();
    }
}

void PhaserFX::SetBypass(bool bypass) {
    bypassed_ = bypass;
    bypassed_setting_value_ = bypass;
    
    // Initialize when enabled (if not already initialized)
    if (!bypass && !initialized_) {
        Init();
    }
}

void PhaserFX::SetWetDry(float wet_dry) {
    wet_dry_ = wet_dry;
    if (wet_dry_ < 0.0f) wet_dry_ = 0.0f;
    if (wet_dry_ > 1.0f) wet_dry_ = 1.0f;
    wet_dry_setting_value_ = wet_dry_;
}

void PhaserFX::InitializeSettings() {
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
    
    // Allpass Frequency (float 100-10000 Hz)
    settings_[2].name = "AP Freq";
    settings_[2].type = SettingType::FLOAT;
    settings_[2].value_ptr = &ap_freq_setting_value_;
    settings_[2].min_value = 100.0f;
    settings_[2].max_value = 10000.0f;
    settings_[2].step_size = 50.0f;
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
    
    // Poles (float 1-8, displayed as int)
    settings_[4].name = "Poles";
    settings_[4].type = SettingType::FLOAT;
    settings_[4].value_ptr = &poles_setting_value_;
    settings_[4].min_value = 1.0f;
    settings_[4].max_value = 8.0f;
    settings_[4].step_size = 1.0f;
    settings_[4].enum_options = nullptr;
    settings_[4].enum_count = 0;
    settings_[4].on_change_callback = nullptr;
    
    // Wet/Dry (float 0-1)
    settings_[5].name = "Wet/Dry";
    settings_[5].type = SettingType::FLOAT;
    settings_[5].value_ptr = &wet_dry_setting_value_;
    settings_[5].min_value = 0.0f;
    settings_[5].max_value = 1.0f;
    settings_[5].step_size = 0.01f;
    settings_[5].enum_options = nullptr;
    settings_[5].enum_count = 0;
    settings_[5].on_change_callback = nullptr;
    
    // Bypass (bool)
    settings_[6].name = "Bypass";
    settings_[6].type = SettingType::BOOL;
    settings_[6].value_ptr = &bypassed_setting_value_;
    settings_[6].min_value = 0.0f;
    settings_[6].max_value = 1.0f;
    settings_[6].step_size = 1.0f;
    settings_[6].enum_options = nullptr;
    settings_[6].enum_count = 0;
    settings_[6].on_change_callback = nullptr;
}

int PhaserFX::GetSettingCount() const {
    return 7;
}

const PluginSetting* PhaserFX::GetSetting(int index) const {
    if (index >= 0 && index < GetSettingCount()) {
        return &settings_[index];
    }
    return nullptr;
}

void PhaserFX::OnSettingChanged(int setting_index) {
    lfo_depth_ = lfo_depth_setting_value_;
    lfo_freq_ = lfo_freq_setting_value_;
    ap_freq_ = ap_freq_setting_value_;
    feedback_ = feedback_setting_value_;
    poles_ = static_cast<int>(poles_setting_value_ + 0.5f);  // Round to nearest int
    if (poles_ < 1) poles_ = 1;
    if (poles_ > 8) poles_ = 8;
    wet_dry_ = wet_dry_setting_value_;
    bypassed_ = bypassed_setting_value_;
    
    UpdatePhaserParams();
}

void PhaserFX::UpdatePhaserParams() {
    if (!initialized_) return;
    
    phaser_.SetPoles(poles_);
    phaser_.SetLfoDepth(lfo_depth_);
    phaser_.SetLfoFreq(lfo_freq_);
    phaser_.SetFreq(ap_freq_);
    phaser_.SetFeedback(feedback_);
}

void PhaserFX::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    struct State {
        float lfo_depth;
        float lfo_freq;
        float ap_freq;
        float feedback;
        int poles;
        float wet_dry;
        bool bypassed;
    };
    
    State state;
    state.lfo_depth = lfo_depth_;
    state.lfo_freq = lfo_freq_;
    state.ap_freq = ap_freq_;
    state.feedback = feedback_;
    state.poles = poles_;
    state.wet_dry = wet_dry_;
    state.bypassed = bypassed_;
    
    std::memcpy(buffer, &state, sizeof(State));
    *size = sizeof(State);
}

void PhaserFX::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(float) * 5 + sizeof(int) + sizeof(bool)) return;
    
    struct State {
        float lfo_depth;
        float lfo_freq;
        float ap_freq;
        float feedback;
        int poles;
        float wet_dry;
        bool bypassed;
    };
    
    const State* state = reinterpret_cast<const State*>(buffer);
    lfo_depth_ = state->lfo_depth;
    lfo_freq_ = state->lfo_freq;
    ap_freq_ = state->ap_freq;
    feedback_ = state->feedback;
    poles_ = state->poles;
    wet_dry_ = state->wet_dry;
    bypassed_ = state->bypassed;
    
    // Update setting values
    lfo_depth_setting_value_ = lfo_depth_;
    lfo_freq_setting_value_ = lfo_freq_;
    ap_freq_setting_value_ = ap_freq_;
    feedback_setting_value_ = feedback_;
    poles_setting_value_ = static_cast<float>(poles_);
    wet_dry_setting_value_ = wet_dry_;
    bypassed_setting_value_ = bypassed_;
    
    // Update effect parameters
    if (initialized_) {
        UpdatePhaserParams();
    }
}

size_t PhaserFX::GetStateSize() const {
    return sizeof(float) * 5 + sizeof(int) + sizeof(bool);
}

} // namespace OpenChord

