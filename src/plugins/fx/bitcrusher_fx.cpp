#include "bitcrusher_fx.h"
#include <cmath>
#include <cstring>

namespace OpenChord {

BitcrusherFX::BitcrusherFX()
    : sample_rate_(48000.0f)
    , bitcrush_factor_(0.5f)    // Default 50% bitcrush (8-bit)
    , downsample_factor_(0.5f)  // Default 50% downsample
    , wet_dry_(0.5f)            // Default 50/50 mix
    , bypassed_(true)           // Start bypassed (off by default)
    , bitcrush_factor_setting_value_(0.5f)
    , downsample_factor_setting_value_(0.5f)
    , wet_dry_setting_value_(0.5f)
    , bypassed_setting_value_(true)  // Start bypassed (off by default)
    , initialized_(false)
{
    InitializeSettings();
}

BitcrusherFX::~BitcrusherFX() {
}

void BitcrusherFX::Init() {
    if (sample_rate_ <= 0.0f) {
        sample_rate_ = 48000.0f;
    }
    
    decimator_.Init();
    initialized_ = true;  // Set before UpdateBitcrusherParams so it can run
    UpdateBitcrusherParams();
}

void BitcrusherFX::Process(const float* const* in, float* const* out, size_t size) {
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
        
        // Process through bitcrusher
        float processed = decimator_.Process(in_sample);
        
        // Mix wet/dry
        float wet = processed * wet_dry_;
        float dry = in_sample * (1.0f - wet_dry_);
        float output = wet + dry;
        
        // Output stereo (same signal to both channels)
        out[0][i] = output;
        out[1][i] = output;
    }
}

void BitcrusherFX::Update() {
    UpdateBitcrusherParams();
}

void BitcrusherFX::UpdateUI() {
    // UI handled by settings system
}

void BitcrusherFX::HandleEncoder(int encoder, float delta) {
    (void)encoder;
    (void)delta;
}

void BitcrusherFX::HandleButton(int button, bool pressed) {
    (void)button;
    (void)pressed;
}

void BitcrusherFX::HandleJoystick(float x, float y) {
    (void)x;
    (void)y;
}

void BitcrusherFX::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    if (initialized_) {
        Init();
    }
}

void BitcrusherFX::SetBypass(bool bypass) {
    bypassed_ = bypass;
    bypassed_setting_value_ = bypass;
    
    // Initialize when enabled (if not already initialized)
    if (!bypass && !initialized_) {
        Init();
    }
}

void BitcrusherFX::SetWetDry(float wet_dry) {
    wet_dry_ = wet_dry;
    if (wet_dry_ < 0.0f) wet_dry_ = 0.0f;
    if (wet_dry_ > 1.0f) wet_dry_ = 1.0f;
    wet_dry_setting_value_ = wet_dry_;
}

void BitcrusherFX::InitializeSettings() {
    // Bitcrush Factor (float 0-1)
    settings_[0].name = "Bitcrush";
    settings_[0].type = SettingType::FLOAT;
    settings_[0].value_ptr = &bitcrush_factor_setting_value_;
    settings_[0].min_value = 0.0f;
    settings_[0].max_value = 1.0f;
    settings_[0].step_size = 0.01f;
    settings_[0].enum_options = nullptr;
    settings_[0].enum_count = 0;
    settings_[0].on_change_callback = nullptr;
    
    // Downsample Factor (float 0-1)
    settings_[1].name = "Downsample";
    settings_[1].type = SettingType::FLOAT;
    settings_[1].value_ptr = &downsample_factor_setting_value_;
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

int BitcrusherFX::GetSettingCount() const {
    return 4;
}

const PluginSetting* BitcrusherFX::GetSetting(int index) const {
    if (index >= 0 && index < GetSettingCount()) {
        return &settings_[index];
    }
    return nullptr;
}

void BitcrusherFX::OnSettingChanged(int setting_index) {
    bitcrush_factor_ = bitcrush_factor_setting_value_;
    downsample_factor_ = downsample_factor_setting_value_;
    wet_dry_ = wet_dry_setting_value_;
    bypassed_ = bypassed_setting_value_;
    
    UpdateBitcrusherParams();
}

void BitcrusherFX::UpdateBitcrusherParams() {
    if (!initialized_) return;
    
    decimator_.SetBitcrushFactor(bitcrush_factor_);
    decimator_.SetDownsampleFactor(downsample_factor_);
}

void BitcrusherFX::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    struct State {
        float bitcrush_factor;
        float downsample_factor;
        float wet_dry;
        bool bypassed;
    };
    
    State state;
    state.bitcrush_factor = bitcrush_factor_;
    state.downsample_factor = downsample_factor_;
    state.wet_dry = wet_dry_;
    state.bypassed = bypassed_;
    
    std::memcpy(buffer, &state, sizeof(State));
    *size = sizeof(State);
}

void BitcrusherFX::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(float) * 3 + sizeof(bool)) return;
    
    struct State {
        float bitcrush_factor;
        float downsample_factor;
        float wet_dry;
        bool bypassed;
    };
    
    const State* state = reinterpret_cast<const State*>(buffer);
    bitcrush_factor_ = state->bitcrush_factor;
    downsample_factor_ = state->downsample_factor;
    wet_dry_ = state->wet_dry;
    bypassed_ = state->bypassed;
    
    // Update setting values
    bitcrush_factor_setting_value_ = bitcrush_factor_;
    downsample_factor_setting_value_ = downsample_factor_;
    wet_dry_setting_value_ = wet_dry_;
    bypassed_setting_value_ = bypassed_;
    
    // Update effect parameters
    if (initialized_) {
        UpdateBitcrusherParams();
    }
}

size_t BitcrusherFX::GetStateSize() const {
    return sizeof(float) * 3 + sizeof(bool);
}

} // namespace OpenChord

