#include "overdrive_fx.h"
#include <cmath>
#include <cstring>

namespace OpenChord {

OverdriveFX::OverdriveFX()
    : sample_rate_(48000.0f)
    , drive_(0.7f)          // Default 70% drive (musical overdrive)
    , wet_dry_(0.8f)        // Default 80% wet (more drive in mix)
    , bypassed_(true)       // Start bypassed (off by default)
    , drive_setting_value_(0.7f)
    , wet_dry_setting_value_(0.8f)
    , bypassed_setting_value_(true)  // Start bypassed (off by default)
    , initialized_(false)
{
    InitializeSettings();
}

OverdriveFX::~OverdriveFX() {
}

void OverdriveFX::Init() {
    if (sample_rate_ <= 0.0f) {
        sample_rate_ = 48000.0f;
    }
    
    overdrive_.Init();
    UpdateOverdriveParams();
    
    initialized_ = true;
}

void OverdriveFX::Process(const float* const* in, float* const* out, size_t size) {
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
        
        // Process through overdrive
        float processed = overdrive_.Process(in_sample);
        
        // Mix wet/dry
        float wet = processed * wet_dry_;
        float dry = in_sample * (1.0f - wet_dry_);
        float output = wet + dry;
        
        // Output stereo (same signal to both channels)
        out[0][i] = output;
        out[1][i] = output;
    }
}

void OverdriveFX::Update() {
    UpdateOverdriveParams();
}

void OverdriveFX::UpdateUI() {
    // UI handled by settings system
}

void OverdriveFX::HandleEncoder(int encoder, float delta) {
    (void)encoder;
    (void)delta;
}

void OverdriveFX::HandleButton(int button, bool pressed) {
    (void)button;
    (void)pressed;
}

void OverdriveFX::HandleJoystick(float x, float y) {
    (void)x;
    (void)y;
}

void OverdriveFX::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    if (initialized_) {
        Init();
    }
}

void OverdriveFX::SetBypass(bool bypass) {
    bypassed_ = bypass;
    bypassed_setting_value_ = bypass;
    
    // Initialize when enabled (if not already initialized)
    if (!bypass && !initialized_) {
        Init();
    }
}

void OverdriveFX::SetWetDry(float wet_dry) {
    wet_dry_ = wet_dry;
    if (wet_dry_ < 0.0f) wet_dry_ = 0.0f;
    if (wet_dry_ > 1.0f) wet_dry_ = 1.0f;
    wet_dry_setting_value_ = wet_dry_;
}

void OverdriveFX::InitializeSettings() {
    // Drive (float 0-1)
    settings_[0].name = "Drive";
    settings_[0].type = SettingType::FLOAT;
    settings_[0].value_ptr = &drive_setting_value_;
    settings_[0].min_value = 0.0f;
    settings_[0].max_value = 1.0f;
    settings_[0].step_size = 0.01f;
    settings_[0].enum_options = nullptr;
    settings_[0].enum_count = 0;
    settings_[0].on_change_callback = nullptr;
    
    // Wet/Dry (float 0-1)
    settings_[1].name = "Wet/Dry";
    settings_[1].type = SettingType::FLOAT;
    settings_[1].value_ptr = &wet_dry_setting_value_;
    settings_[1].min_value = 0.0f;
    settings_[1].max_value = 1.0f;
    settings_[1].step_size = 0.01f;
    settings_[1].enum_options = nullptr;
    settings_[1].enum_count = 0;
    settings_[1].on_change_callback = nullptr;
    
    // Bypass (bool)
    settings_[2].name = "Bypass";
    settings_[2].type = SettingType::BOOL;
    settings_[2].value_ptr = &bypassed_setting_value_;
    settings_[2].min_value = 0.0f;
    settings_[2].max_value = 1.0f;
    settings_[2].step_size = 1.0f;
    settings_[2].enum_options = nullptr;
    settings_[2].enum_count = 0;
    settings_[2].on_change_callback = nullptr;
}

int OverdriveFX::GetSettingCount() const {
    return 3;
}

const PluginSetting* OverdriveFX::GetSetting(int index) const {
    if (index >= 0 && index < GetSettingCount()) {
        return &settings_[index];
    }
    return nullptr;
}

void OverdriveFX::OnSettingChanged(int setting_index) {
    drive_ = drive_setting_value_;
    wet_dry_ = wet_dry_setting_value_;
    bypassed_ = bypassed_setting_value_;
    
    UpdateOverdriveParams();
}

void OverdriveFX::UpdateOverdriveParams() {
    if (!initialized_) return;
    
    overdrive_.SetDrive(drive_);
}

void OverdriveFX::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    struct State {
        float drive;
        float wet_dry;
        bool bypassed;
    };
    
    State state;
    state.drive = drive_;
    state.wet_dry = wet_dry_;
    state.bypassed = bypassed_;
    
    std::memcpy(buffer, &state, sizeof(State));
    *size = sizeof(State);
}

void OverdriveFX::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(float) * 2 + sizeof(bool)) return;
    
    struct State {
        float drive;
        float wet_dry;
        bool bypassed;
    };
    
    const State* state = reinterpret_cast<const State*>(buffer);
    drive_ = state->drive;
    wet_dry_ = state->wet_dry;
    bypassed_ = state->bypassed;
    
    // Update setting values
    drive_setting_value_ = drive_;
    wet_dry_setting_value_ = wet_dry_;
    bypassed_setting_value_ = bypassed_;
    
    // Update effect parameters
    if (initialized_) {
        UpdateOverdriveParams();
    }
}

size_t OverdriveFX::GetStateSize() const {
    return sizeof(float) * 2 + sizeof(bool);
}

} // namespace OpenChord

