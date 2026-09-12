#include "autowah_fx.h"
#include <cmath>
#include <cstring>

namespace OpenChord {

AutowahFX::AutowahFX()
    : sample_rate_(48000.0f)
    , wah_(0.5f)             // Default 50% wah
    , level_(0.7f)           // Default 70% level
    , wet_dry_(0.5f)         // Default 50/50 mix
    , bypassed_(true)        // Start bypassed (off by default)
    , wah_setting_value_(0.5f)
    , level_setting_value_(0.7f)
    , wet_dry_setting_value_(0.5f)
    , bypassed_setting_value_(true)  // Start bypassed (off by default)
    , initialized_(false)
{
    InitializeSettings();
}

AutowahFX::~AutowahFX() {
}

void AutowahFX::Init() {
    if (sample_rate_ <= 0.0f) {
        sample_rate_ = 48000.0f;
    }
    
    autowah_.Init(sample_rate_);
    initialized_ = true;  // Set before UpdateAutowahParams so it can run
    UpdateAutowahParams();
}

void AutowahFX::Process(const float* const* in, float* const* out, size_t size) {
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
        
        // Process through autowah
        float processed = autowah_.Process(in_sample);
        
        // Mix wet/dry
        float wet = processed * wet_dry_;
        float dry = in_sample * (1.0f - wet_dry_);
        float output = wet + dry;
        
        // Output stereo (same signal to both channels)
        out[0][i] = output;
        out[1][i] = output;
    }
}

void AutowahFX::Update() {
    UpdateAutowahParams();
}

void AutowahFX::UpdateUI() {
    // UI handled by settings system
}

void AutowahFX::HandleEncoder(int encoder, float delta) {
    (void)encoder;
    (void)delta;
}

void AutowahFX::HandleButton(int button, bool pressed) {
    (void)button;
    (void)pressed;
}

void AutowahFX::HandleJoystick(float x, float y) {
    (void)x;
    (void)y;
}

void AutowahFX::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    if (initialized_) {
        Init();
    }
}

void AutowahFX::SetBypass(bool bypass) {
    bypassed_ = bypass;
    bypassed_setting_value_ = bypass;
    
    // Initialize when enabled (if not already initialized)
    if (!bypass && !initialized_) {
        Init();
    }
}

void AutowahFX::SetWetDry(float wet_dry) {
    wet_dry_ = wet_dry;
    if (wet_dry_ < 0.0f) wet_dry_ = 0.0f;
    if (wet_dry_ > 1.0f) wet_dry_ = 1.0f;
    wet_dry_setting_value_ = wet_dry_;
}

void AutowahFX::InitializeSettings() {
    // Wah (float 0-1)
    settings_[0].name = "Wah";
    settings_[0].type = SettingType::FLOAT;
    settings_[0].value_ptr = &wah_setting_value_;
    settings_[0].min_value = 0.0f;
    settings_[0].max_value = 1.0f;
    settings_[0].step_size = 0.01f;
    settings_[0].enum_options = nullptr;
    settings_[0].enum_count = 0;
    settings_[0].on_change_callback = nullptr;
    
    // Level (float 0-1)
    settings_[1].name = "Level";
    settings_[1].type = SettingType::FLOAT;
    settings_[1].value_ptr = &level_setting_value_;
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

int AutowahFX::GetSettingCount() const {
    return 4;
}

const PluginSetting* AutowahFX::GetSetting(int index) const {
    if (index >= 0 && index < GetSettingCount()) {
        return &settings_[index];
    }
    return nullptr;
}

void AutowahFX::OnSettingChanged(int setting_index) {
    wah_ = wah_setting_value_;
    level_ = level_setting_value_;
    wet_dry_ = wet_dry_setting_value_;
    bypassed_ = bypassed_setting_value_;
    
    UpdateAutowahParams();
}

void AutowahFX::UpdateAutowahParams() {
    if (!initialized_) return;
    
    autowah_.SetWah(wah_);
    autowah_.SetLevel(level_);
    // Autowah uses 0-100 for dry/wet, so convert from 0-1
    autowah_.SetDryWet(wet_dry_ * 100.0f);
}

void AutowahFX::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    struct State {
        float wah;
        float level;
        float wet_dry;
        bool bypassed;
    };
    
    State state;
    state.wah = wah_;
    state.level = level_;
    state.wet_dry = wet_dry_;
    state.bypassed = bypassed_;
    
    std::memcpy(buffer, &state, sizeof(State));
    *size = sizeof(State);
}

void AutowahFX::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(float) * 3 + sizeof(bool)) return;
    
    struct State {
        float wah;
        float level;
        float wet_dry;
        bool bypassed;
    };
    
    const State* state = reinterpret_cast<const State*>(buffer);
    wah_ = state->wah;
    level_ = state->level;
    wet_dry_ = state->wet_dry;
    bypassed_ = state->bypassed;
    
    // Update setting values
    wah_setting_value_ = wah_;
    level_setting_value_ = level_;
    wet_dry_setting_value_ = wet_dry_;
    bypassed_setting_value_ = bypassed_;
    
    // Update effect parameters
    if (initialized_) {
        UpdateAutowahParams();
    }
}

size_t AutowahFX::GetStateSize() const {
    return sizeof(float) * 3 + sizeof(bool);
}

} // namespace OpenChord

