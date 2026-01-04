#include "tremolo_fx.h"
#include <cmath>
#include <cstring>

namespace OpenChord {

const char* TremoloFX::waveform_names_[] = {
    "Sine",
    "Triangle",
    "Square",
    "Saw",
    nullptr
};

TremoloFX::TremoloFX()
    : sample_rate_(48000.0f)
    , rate_(3.0f)           // Default 3 Hz (musical tremolo speed)
    , depth_(0.6f)          // Default 60% depth (noticeable but not extreme)
    , waveform_(daisysp::Oscillator::WAVE_SIN)  // Default sine (smooth)
    , wet_dry_(0.7f)         // Default 70% wet (more effect in mix)
    , bypassed_(true)        // Start bypassed (off by default)
    , rate_setting_value_(3.0f)
    , depth_setting_value_(0.6f)
    , waveform_setting_value_(daisysp::Oscillator::WAVE_SIN)
    , wet_dry_setting_value_(0.7f)
    , bypassed_setting_value_(true)  // Start bypassed (off by default)
    , initialized_(false)
{
    InitializeSettings();
}

TremoloFX::~TremoloFX() {
}

void TremoloFX::Init() {
    if (sample_rate_ <= 0.0f) {
        sample_rate_ = 48000.0f;
    }
    
    tremolo_.Init(sample_rate_);
    UpdateTremoloParams();
    
    initialized_ = true;
}

void TremoloFX::Process(const float* const* in, float* const* out, size_t size) {
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
        
        // Process through tremolo
        float processed = tremolo_.Process(in_sample);
        
        // Mix wet/dry
        float wet = processed * wet_dry_;
        float dry = in_sample * (1.0f - wet_dry_);
        float output = wet + dry;
        
        // Output stereo (same signal to both channels)
        out[0][i] = output;
        out[1][i] = output;
    }
}

void TremoloFX::Update() {
    UpdateTremoloParams();
}

void TremoloFX::UpdateUI() {
    // UI handled by settings system
}

void TremoloFX::HandleEncoder(int encoder, float delta) {
    (void)encoder;
    (void)delta;
}

void TremoloFX::HandleButton(int button, bool pressed) {
    (void)button;
    (void)pressed;
}

void TremoloFX::HandleJoystick(float x, float y) {
    (void)x;
    (void)y;
}

void TremoloFX::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    if (initialized_) {
        Init();
    }
}

void TremoloFX::SetBypass(bool bypass) {
    bypassed_ = bypass;
    bypassed_setting_value_ = bypass;
    
    // Initialize when enabled (if not already initialized)
    if (!bypass && !initialized_) {
        Init();
    }
}

void TremoloFX::SetWetDry(float wet_dry) {
    wet_dry_ = wet_dry;
    if (wet_dry_ < 0.0f) wet_dry_ = 0.0f;
    if (wet_dry_ > 1.0f) wet_dry_ = 1.0f;
    wet_dry_setting_value_ = wet_dry_;
}

void TremoloFX::InitializeSettings() {
    // Rate (float 0.1-10 Hz)
    settings_[0].name = "Rate";
    settings_[0].type = SettingType::FLOAT;
    settings_[0].value_ptr = &rate_setting_value_;
    settings_[0].min_value = 0.1f;
    settings_[0].max_value = 10.0f;
    settings_[0].step_size = 0.1f;
    settings_[0].enum_options = nullptr;
    settings_[0].enum_count = 0;
    settings_[0].on_change_callback = nullptr;
    
    // Depth (float 0-1)
    settings_[1].name = "Depth";
    settings_[1].type = SettingType::FLOAT;
    settings_[1].value_ptr = &depth_setting_value_;
    settings_[1].min_value = 0.0f;
    settings_[1].max_value = 1.0f;
    settings_[1].step_size = 0.01f;
    settings_[1].enum_options = nullptr;
    settings_[1].enum_count = 0;
    settings_[1].on_change_callback = nullptr;
    
    // Waveform (enum)
    settings_[2].name = "Waveform";
    settings_[2].type = SettingType::ENUM;
    settings_[2].value_ptr = &waveform_setting_value_;
    settings_[2].min_value = 0.0f;
    settings_[2].max_value = 3.0f;  // 4 waveforms: 0-3
    settings_[2].step_size = 1.0f;
    settings_[2].enum_options = waveform_names_;
    settings_[2].enum_count = 4;
    settings_[2].on_change_callback = nullptr;
    
    // Wet/Dry (float 0-1)
    settings_[3].name = "Wet/Dry";
    settings_[3].type = SettingType::FLOAT;
    settings_[3].value_ptr = &wet_dry_setting_value_;
    settings_[3].min_value = 0.0f;
    settings_[3].max_value = 1.0f;
    settings_[3].step_size = 0.01f;
    settings_[3].enum_options = nullptr;
    settings_[3].enum_count = 0;
    settings_[3].on_change_callback = nullptr;
    
    // Bypass (bool)
    settings_[4].name = "Bypass";
    settings_[4].type = SettingType::BOOL;
    settings_[4].value_ptr = &bypassed_setting_value_;
    settings_[4].min_value = 0.0f;
    settings_[4].max_value = 1.0f;
    settings_[4].step_size = 1.0f;
    settings_[4].enum_options = nullptr;
    settings_[4].enum_count = 0;
    settings_[4].on_change_callback = nullptr;
}

int TremoloFX::GetSettingCount() const {
    return 5;
}

const PluginSetting* TremoloFX::GetSetting(int index) const {
    if (index >= 0 && index < GetSettingCount()) {
        return &settings_[index];
    }
    return nullptr;
}

void TremoloFX::OnSettingChanged(int setting_index) {
    rate_ = rate_setting_value_;
    depth_ = depth_setting_value_;
    waveform_ = waveform_setting_value_;
    wet_dry_ = wet_dry_setting_value_;
    bypassed_ = bypassed_setting_value_;
    
    UpdateTremoloParams();
}

void TremoloFX::UpdateTremoloParams() {
    if (!initialized_) return;
    
    tremolo_.SetFreq(rate_);
    tremolo_.SetDepth(depth_);
    tremolo_.SetWaveform(waveform_);
}

void TremoloFX::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    struct State {
        float rate;
        float depth;
        int waveform;
        float wet_dry;
        bool bypassed;
    };
    
    State state;
    state.rate = rate_;
    state.depth = depth_;
    state.waveform = waveform_;
    state.wet_dry = wet_dry_;
    state.bypassed = bypassed_;
    
    std::memcpy(buffer, &state, sizeof(State));
    *size = sizeof(State);
}

void TremoloFX::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(float) * 3 + sizeof(int) + sizeof(bool)) return;
    
    struct State {
        float rate;
        float depth;
        int waveform;
        float wet_dry;
        bool bypassed;
    };
    
    const State* state = reinterpret_cast<const State*>(buffer);
    rate_ = state->rate;
    depth_ = state->depth;
    waveform_ = state->waveform;
    wet_dry_ = state->wet_dry;
    bypassed_ = state->bypassed;
    
    // Update setting values
    rate_setting_value_ = rate_;
    depth_setting_value_ = depth_;
    waveform_setting_value_ = waveform_;
    wet_dry_setting_value_ = wet_dry_;
    bypassed_setting_value_ = bypassed_;
    
    // Update effect parameters
    if (initialized_) {
        UpdateTremoloParams();
    }
}

size_t TremoloFX::GetStateSize() const {
    return sizeof(float) * 3 + sizeof(int) + sizeof(bool);
}

} // namespace OpenChord

