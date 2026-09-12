#include "encoder_input_handler.h"
#include <cstring>
#include <cmath>

namespace OpenChord {

EncoderInputHandler::EncoderInputHandler()
    : digital_manager_(nullptr),
      mode_(EncoderMode::NAVIGATION),
      acceleration_enabled_(true),
      acceleration_threshold_(3),
      current_value_(0),
      prev_value_(0),
      current_delta_(0.0f),
      current_direction_(EncoderDirection::NONE),
      event_queue_head_(0),
      event_queue_tail_(0),
      event_queue_count_(0),
      rotation_steps_(0),
      last_rotation_time_(0) {
    
    memset(event_queue_, 0, sizeof(event_queue_));
}

EncoderInputHandler::~EncoderInputHandler() {
    digital_manager_ = nullptr;
}

void EncoderInputHandler::Init(::DigitalManager* digital_manager) {
    digital_manager_ = digital_manager;
    mode_ = EncoderMode::NAVIGATION;
    acceleration_enabled_ = true;
    acceleration_threshold_ = 3;
    
    current_value_ = 0;
    prev_value_ = 0;
    current_delta_ = 0.0f;
    current_direction_ = EncoderDirection::NONE;
    
    event_queue_head_ = 0;
    event_queue_tail_ = 0;
    event_queue_count_ = 0;
    
    rotation_steps_ = 0;
    last_rotation_time_ = 0;
    
    // Reset encoder in DigitalManager
    if (digital_manager_) {
        digital_manager_->ResetEncoder(0);
    }
}

void EncoderInputHandler::Update() {
    if (!digital_manager_) return;
    
    ProcessEncoder();
    ProcessButton();
}

void EncoderInputHandler::ProcessEncoder() {
    if (!digital_manager_) return;
    
    // Get encoder value and delta from DigitalManager
    int raw_value = digital_manager_->GetEncoderValue();
    float raw_delta = digital_manager_->GetEncoderDelta();
    
    // Determine direction
    if (raw_delta > 0.0f) {
        current_direction_ = EncoderDirection::CLOCKWISE;
    } else if (raw_delta < 0.0f) {
        current_direction_ = EncoderDirection::COUNTER_CLOCKWISE;
    } else {
        current_direction_ = EncoderDirection::NONE;
    }
    
    // Update value
    if (raw_value != prev_value_) {
        int delta = raw_value - prev_value_;
        uint32_t current_time = digital_manager_->GetCurrentTime();
        
        // Time-based velocity detection for acceleration
        if (acceleration_enabled_) {
            uint32_t time_delta = 0;
            if (last_rotation_time_ > 0) {
                time_delta = current_time - last_rotation_time_;
            }
            
            // If rotation direction matches, calculate velocity
            bool direction_match = (delta > 0 && rotation_steps_ >= 0) || 
                                  (delta < 0 && rotation_steps_ <= 0);
            
            if (direction_match && time_delta > 0 && time_delta < 150) {
                // Fast rotation detected (time_delta < 150ms)
                // Calculate velocity: faster rotation = smaller time_delta = larger multiplier
                // Normalize: 150ms = 1x, 75ms = 2x, 50ms = 3x, 30ms = 5x (capped)
                // Use a more gradual curve for better control
                float velocity_multiplier = 1.0f;
                if (time_delta < 30) {
                    velocity_multiplier = 5.0f;  // Very fast: 5x
                } else if (time_delta < 50) {
                    velocity_multiplier = 3.0f;  // Fast: 3x
                } else if (time_delta < 75) {
                    velocity_multiplier = 2.0f;  // Medium-fast: 2x
                } else if (time_delta < 100) {
                    velocity_multiplier = 1.5f;  // Slightly fast: 1.5x
                } else {
                    velocity_multiplier = 1.0f;  // Normal: 1x
                }
                
                // Apply velocity-based acceleration
                current_delta_ = static_cast<float>(delta) * velocity_multiplier;
                
                // Track rotation steps for direction consistency
                if (delta > 0) {
                    rotation_steps_ = (rotation_steps_ < 0) ? 1 : rotation_steps_ + 1;
                } else {
                    rotation_steps_ = (rotation_steps_ > 0) ? -1 : rotation_steps_ - 1;
                }
            } else {
                // Slow rotation or direction changed - no acceleration
                current_delta_ = static_cast<float>(delta);
                rotation_steps_ = (delta > 0) ? 1 : -1;
            }
            
            last_rotation_time_ = current_time;
        } else {
            // Acceleration disabled - use raw delta
            current_delta_ = static_cast<float>(delta);
            rotation_steps_ = 0;
            last_rotation_time_ = current_time;
        }
        
        current_value_ = raw_value;
        
        // Queue rotation event
        EncoderEvent event;
        event.type = EncoderEventType::ROTATED;
        event.delta = delta;
        event.value = current_value_;
        event.timestamp = current_time;
        QueueEvent(event);
        
        prev_value_ = current_value_;
    } else {
        current_delta_ = 0.0f;
        current_direction_ = EncoderDirection::NONE;
        
        // Reset rotation tracking if no movement for a while
        uint32_t current_time = digital_manager_->GetCurrentTime();
        if (last_rotation_time_ > 0 && (current_time - last_rotation_time_) > 500) {
            rotation_steps_ = 0;
            last_rotation_time_ = 0;
        }
    }
}

void EncoderInputHandler::ProcessButton() {
    if (!digital_manager_) return;
    
    // Check for button press (when implemented)
    // For now, encoder button is not connected, so this is a placeholder
    bool current_pressed = digital_manager_->IsEncoderButtonPressed();
    bool was_pressed = digital_manager_->WasEncoderButtonPressed();
    static bool prev_pressed = false;
    
    // Detect press event
    if (was_pressed || (!prev_pressed && current_pressed)) {
        EncoderEvent event;
        event.type = EncoderEventType::BUTTON_PRESSED;
        event.delta = 0;
        event.value = current_value_;
        event.timestamp = 0;
        QueueEvent(event);
    }
    
    // Detect release event
    if (prev_pressed && !current_pressed) {
        EncoderEvent event;
        event.type = EncoderEventType::BUTTON_RELEASED;
        event.delta = 0;
        event.value = current_value_;
        event.timestamp = 0;
        QueueEvent(event);
    }
    
    // Detect hold event
    if (current_pressed) {
        uint32_t hold_time = digital_manager_->GetEncoderButtonHoldTime();
        if (hold_time > 500 && !prev_pressed) { // 500ms threshold
            EncoderEvent event;
            event.type = EncoderEventType::BUTTON_HELD;
            event.delta = 0;
            event.value = current_value_;
            event.timestamp = 0;
            QueueEvent(event);
        }
    }
    
    prev_pressed = current_pressed;
}

void EncoderInputHandler::QueueEvent(const EncoderEvent& event) {
    if (event_queue_count_ >= MAX_EVENTS) {
        // Queue full, drop oldest event
        event_queue_head_ = (event_queue_head_ + 1) % MAX_EVENTS;
        event_queue_count_--;
    }
    
    event_queue_[event_queue_tail_] = event;
    event_queue_tail_ = (event_queue_tail_ + 1) % MAX_EVENTS;
    event_queue_count_++;
}

bool EncoderInputHandler::PollEvent(EncoderEvent& event) {
    if (event_queue_count_ == 0) {
        return false;
    }
    
    event = event_queue_[event_queue_head_];
    event_queue_head_ = (event_queue_head_ + 1) % MAX_EVENTS;
    event_queue_count_--;
    return true;
}

int EncoderInputHandler::GetValue() const {
    return current_value_;
}

float EncoderInputHandler::GetDelta() const {
    return current_delta_;
}

int EncoderInputHandler::GetRawDelta() const {
    return current_value_ - prev_value_;
}

EncoderDirection EncoderInputHandler::GetDirection() const {
    return current_direction_;
}

bool EncoderInputHandler::IsRotating() const {
    return current_direction_ != EncoderDirection::NONE;
}

bool EncoderInputHandler::IsButtonPressed() const {
    if (!digital_manager_) return false;
    return digital_manager_->IsEncoderButtonPressed();
}

bool EncoderInputHandler::WasButtonPressed() const {
    if (!digital_manager_) return false;
    return digital_manager_->WasEncoderButtonPressed();
}

bool EncoderInputHandler::IsButtonHeld() const {
    if (!digital_manager_) return false;
    return digital_manager_->GetEncoderButtonHoldTime() > 500; // 500ms threshold
}

uint32_t EncoderInputHandler::GetButtonHoldTime() const {
    if (!digital_manager_) return 0;
    return digital_manager_->GetEncoderButtonHoldTime();
}

void EncoderInputHandler::Reset(int value) {
    current_value_ = value;
    prev_value_ = value;
    current_delta_ = 0.0f;
    current_direction_ = EncoderDirection::NONE;
    rotation_steps_ = 0;
    last_rotation_time_ = 0;
    
    if (digital_manager_) {
        digital_manager_->ResetEncoder(value);
    }
}

void EncoderInputHandler::SetAccelerationEnabled(bool enabled) {
    acceleration_enabled_ = enabled;
    if (!enabled) {
        rotation_steps_ = 0;
    }
}

void EncoderInputHandler::SetAccelerationThreshold(int threshold) {
    acceleration_threshold_ = threshold;
    if (acceleration_threshold_ < 1) acceleration_threshold_ = 1;
}

float EncoderInputHandler::CalculateAcceleration(int steps) const {
    if (!acceleration_enabled_ || steps < acceleration_threshold_) {
        return 1.0f;
    }
    
    // Simple acceleration: 1x for first threshold steps, then increase
    // Example: 1x for first 3 steps, 2x for next 3, 3x for next 3, etc.
    int acceleration_level = (steps - acceleration_threshold_) / acceleration_threshold_ + 1;
    
    // Cap acceleration at 5x
    if (acceleration_level > 5) acceleration_level = 5;
    
    return static_cast<float>(acceleration_level);
}

} // namespace OpenChord

