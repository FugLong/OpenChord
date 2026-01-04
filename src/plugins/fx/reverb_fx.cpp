#include "reverb_fx.h"
#include <cmath>
#include <cstring>

namespace OpenChord {

ReverbFX::ReverbFX()
    : sample_rate_(48000.0f)
    , room_size_(0.5f)      // Default medium room
    , damping_(0.5f)       // Default 50% damping (balanced)
    , wet_dry_(0.3f)       // Default 30% wet (subtle reverb)
    , bypassed_(true)      // Start bypassed (off by default)
    , room_size_setting_value_(0.5f)
    , damping_setting_value_(0.5f)
    , wet_dry_setting_value_(0.3f)
    , bypassed_setting_value_(true)  // Start bypassed (off by default)
    , initialized_(false)
{
    // Initialize delay times (prime numbers for better diffusion)
    // Using 2 delay lines now (reduced from 4 to save memory)
    delay_times_[0] = 0.0297f * 48000.0f;  // ~29.7ms
    delay_times_[1] = 0.0371f * 48000.0f;  // ~37.1ms
    // Removed delay_times_[2] and delay_times_[3] to save memory
    
    InitializeSettings();
}

ReverbFX::~ReverbFX() {
}

void ReverbFX::Init() {
    if (sample_rate_ <= 0.0f) {
        sample_rate_ = 48000.0f;
    }
    
    // Initialize all delay lines
    for (size_t i = 0; i < kNumDelays; i++) {
        delay_lines_[i].Init();
    }
    
    initialized_ = true;  // Set before UpdateReverbParams so it can run
    UpdateReverbParams();
}

void ReverbFX::Process(const float* const* in, float* const* out, size_t size) {
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
        
        // Process through multiple delay lines (simple reverb)
        float reverb_sum = 0.0f;
        for (size_t j = 0; j < kNumDelays; j++) {
            float delayed = delay_lines_[j].Read();
            float feedback = delayed * feedback_gains_[j];
            delay_lines_[j].Write(in_sample + feedback);
            reverb_sum += delayed;
        }
        
        // Normalize and apply damping
        reverb_sum = reverb_sum / static_cast<float>(kNumDelays) * (1.0f - damping_);
        
        // Mix wet/dry
        float wet = reverb_sum * wet_dry_;
        float dry = in_sample * (1.0f - wet_dry_);
        float output = wet + dry;
        
        // Output stereo (same signal to both channels)
        out[0][i] = output;
        out[1][i] = output;
    }
}

void ReverbFX::Update() {
    UpdateReverbParams();
}

void ReverbFX::UpdateUI() {
    // UI handled by settings system
}

void ReverbFX::HandleEncoder(int encoder, float delta) {
    (void)encoder;
    (void)delta;
}

void ReverbFX::HandleButton(int button, bool pressed) {
    (void)button;
    (void)pressed;
}

void ReverbFX::HandleJoystick(float x, float y) {
    (void)x;
    (void)y;
}

void ReverbFX::SetSampleRate(float sample_rate) {
    sample_rate_ = sample_rate;
    if (initialized_) {
        Init();
    }
}

void ReverbFX::SetBypass(bool bypass) {
    bypassed_ = bypass;
    bypassed_setting_value_ = bypass;
    
    // Initialize when enabled (if not already initialized)
    if (!bypass && !initialized_) {
        Init();
    }
}

void ReverbFX::SetWetDry(float wet_dry) {
    wet_dry_ = wet_dry;
    if (wet_dry_ < 0.0f) wet_dry_ = 0.0f;
    if (wet_dry_ > 1.0f) wet_dry_ = 1.0f;
    wet_dry_setting_value_ = wet_dry_;
}

void ReverbFX::InitializeSettings() {
    // Room Size (float 0-1)
    settings_[0].name = "Room Size";
    settings_[0].type = SettingType::FLOAT;
    settings_[0].value_ptr = &room_size_setting_value_;
    settings_[0].min_value = 0.0f;
    settings_[0].max_value = 1.0f;
    settings_[0].step_size = 0.01f;
    settings_[0].enum_options = nullptr;
    settings_[0].enum_count = 0;
    settings_[0].on_change_callback = nullptr;
    
    // Damping (float 0-1)
    settings_[1].name = "Damping";
    settings_[1].type = SettingType::FLOAT;
    settings_[1].value_ptr = &damping_setting_value_;
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

int ReverbFX::GetSettingCount() const {
    return 4;
}

const PluginSetting* ReverbFX::GetSetting(int index) const {
    if (index >= 0 && index < GetSettingCount()) {
        return &settings_[index];
    }
    return nullptr;
}

void ReverbFX::OnSettingChanged(int setting_index) {
    room_size_ = room_size_setting_value_;
    damping_ = damping_setting_value_;
    wet_dry_ = wet_dry_setting_value_;
    bypassed_ = bypassed_setting_value_;
    
    UpdateReverbParams();
}

void ReverbFX::UpdateReverbParams() {
    if (!initialized_) return;
    
    // Update delay times based on room size
    float base_delay_scale = 0.5f + room_size_ * 1.5f;  // 0.5x to 2.0x
    
    for (size_t i = 0; i < kNumDelays; i++) {
        float delay_samples = delay_times_[i] * base_delay_scale;
        if (delay_samples < 1.0f) delay_samples = 1.0f;
        if (delay_samples > static_cast<float>(kMaxDelaySamples)) {
            delay_samples = static_cast<float>(kMaxDelaySamples);
        }
        delay_lines_[i].SetDelay(delay_samples);
        
        // Feedback gain based on room size and damping
        feedback_gains_[i] = (0.3f + room_size_ * 0.4f) * (1.0f - damping_ * 0.5f);
        if (feedback_gains_[i] > 0.95f) feedback_gains_[i] = 0.95f;  // Prevent instability
    }
}

void ReverbFX::SaveState(void* buffer, size_t* size) const {
    if (!buffer || !size) return;
    
    struct State {
        float room_size;
        float damping;
        float wet_dry;
        bool bypassed;
    };
    
    State state;
    state.room_size = room_size_;
    state.damping = damping_;
    state.wet_dry = wet_dry_;
    state.bypassed = bypassed_;
    
    std::memcpy(buffer, &state, sizeof(State));
    *size = sizeof(State);
}

void ReverbFX::LoadState(const void* buffer, size_t size) {
    if (!buffer || size < sizeof(float) * 3 + sizeof(bool)) return;
    
    struct State {
        float room_size;
        float damping;
        float wet_dry;
        bool bypassed;
    };
    
    const State* state = reinterpret_cast<const State*>(buffer);
    room_size_ = state->room_size;
    damping_ = state->damping;
    wet_dry_ = state->wet_dry;
    bypassed_ = state->bypassed;
    
    // Update setting values
    room_size_setting_value_ = room_size_;
    damping_setting_value_ = damping_;
    wet_dry_setting_value_ = wet_dry_;
    bypassed_setting_value_ = bypassed_;
    
    // Update effect parameters
    if (initialized_) {
        UpdateReverbParams();
    }
}

size_t ReverbFX::GetStateSize() const {
    return sizeof(float) * 3 + sizeof(bool);
}

} // namespace OpenChord

