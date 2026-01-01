#include "power_manager.h"
#include <algorithm>

namespace OpenChord {

PowerManager::PowerManager()
    : hw_(nullptr)
    , last_activity_time_(0)
    , last_user_input_time_(0)
    , last_audio_activity_time_(0)
    , current_mode_(PowerMode::NORMAL)
    , mode_change_time_(0)
{
}

PowerManager::~PowerManager() {
    hw_ = nullptr;
}

void PowerManager::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    boot_start_time_ = hw_ ? hw_->system.GetNow() : 0;
    last_activity_time_ = boot_start_time_;
    last_user_input_time_ = boot_start_time_;
    last_audio_activity_time_ = boot_start_time_;
    current_mode_ = PowerMode::NORMAL;
    mode_change_time_ = boot_start_time_;
}

void PowerManager::ReportActivity() {
    if (!hw_) return;
    last_activity_time_ = hw_->system.GetNow();
}

void PowerManager::ReportUserInput() {
    if (!hw_) return;
    last_user_input_time_ = hw_->system.GetNow();
    last_activity_time_ = last_user_input_time_;
}

void PowerManager::ReportAudioActivity() {
    if (!hw_) return;
    last_audio_activity_time_ = hw_->system.GetNow();
    // Audio activity is less critical than user input, but still counts
    if (current_mode_ == PowerMode::IDLE || current_mode_ == PowerMode::LOW) {
        last_activity_time_ = last_audio_activity_time_;
    }
}

uint32_t PowerManager::Update() {
    if (!hw_) return MAIN_LOOP_NORMAL_MS;
    
    UpdatePowerMode();
    return GetMainLoopInterval();
}

void PowerManager::UpdatePowerMode() {
    PowerMode new_mode = DeterminePowerMode();
    
    if (new_mode != current_mode_) {
        current_mode_ = new_mode;
        mode_change_time_ = hw_->system.GetNow();
    }
}

PowerManager::PowerMode PowerManager::DeterminePowerMode() const {
    if (!hw_) return PowerMode::NORMAL;
    
    uint32_t now = hw_->system.GetNow();
    
    // During boot period, always stay in NORMAL mode (don't optimize too early)
    uint32_t time_since_boot = now - boot_start_time_;
    if (time_since_boot < BOOT_PERIOD_MS) {
        return PowerMode::NORMAL;
    }
    
    uint32_t time_since_activity = now - last_activity_time_;
    uint32_t time_since_user_input = now - last_user_input_time_;
    
    // Very recent user input = ACTIVE
    if (time_since_user_input < ACTIVE_THRESHOLD_MS) {
        return PowerMode::ACTIVE;
    }
    
    // Recent activity = NORMAL
    if (time_since_activity < LOW_THRESHOLD_MS) {
        return PowerMode::NORMAL;
    }
    
    // Some activity but not recent = LOW
    if (time_since_activity < IDLE_THRESHOLD_MS) {
        return PowerMode::LOW;
    }
    
    // No activity for a while = IDLE
    return PowerMode::IDLE;
}

uint32_t PowerManager::GetMainLoopInterval() const {
    switch (current_mode_) {
        case PowerMode::IDLE:   return MAIN_LOOP_IDLE_MS;
        case PowerMode::LOW:    return MAIN_LOOP_LOW_MS;
        case PowerMode::NORMAL: return MAIN_LOOP_NORMAL_MS;
        case PowerMode::ACTIVE: return MAIN_LOOP_ACTIVE_MS;
        default:                return MAIN_LOOP_NORMAL_MS;
    }
}

uint32_t PowerManager::GetADCInterval() const {
    switch (current_mode_) {
        case PowerMode::IDLE:   return ADC_IDLE_MS;
        case PowerMode::LOW:    return ADC_LOW_MS;
        case PowerMode::NORMAL: return ADC_NORMAL_MS;
        case PowerMode::ACTIVE: return ADC_ACTIVE_MS;
        default:                return ADC_NORMAL_MS;
    }
}

uint32_t PowerManager::GetDisplayInterval() const {
    switch (current_mode_) {
        case PowerMode::IDLE:   return DISPLAY_IDLE_MS;
        case PowerMode::LOW:    return DISPLAY_LOW_MS;
        case PowerMode::NORMAL: return DISPLAY_NORMAL_MS;
        case PowerMode::ACTIVE: return DISPLAY_ACTIVE_MS;
        default:                return DISPLAY_NORMAL_MS;
    }
}

bool PowerManager::ShouldUpdateADC(uint32_t last_update_time) const {
    if (!hw_) return false;
    uint32_t now = hw_->system.GetNow();
    uint32_t interval = GetADCInterval();
    return (now - last_update_time) >= interval;
}

bool PowerManager::ShouldUpdateDisplay(uint32_t last_update_time) const {
    if (!hw_) return false;
    uint32_t now = hw_->system.GetNow();
    uint32_t interval = GetDisplayInterval();
    return (now - last_update_time) >= interval;
}

} // namespace OpenChord

