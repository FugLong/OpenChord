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
        
        // Apply acceleration if enabled
        if (acceleration_enabled_ && abs(delta) >= acceleration_threshold_) {
            // Track consecutive rotations
            if (delta > 0 && rotation_steps_ >= 0) {
                rotation_steps_++;
            } else if (delta < 0 && rotation_steps_ <= 0) {
                rotation_steps_--;
            } else {
                rotation_steps_ = delta; // Direction changed, reset
            }
            
            float acceleration = CalculateAcceleration(abs(rotation_steps_));
            current_delta_ = static_cast<float>(delta) * acceleration;
        } else {
            current_delta_ = static_cast<float>(delta);
            rotation_steps_ = 0;
        }
        
        current_value_ = raw_value;
        
        // Queue rotation event
        EncoderEvent event;
        event.type = EncoderEventType::ROTATED;
        event.delta = delta;
        event.value = current_value_;
        event.timestamp = 0; // TODO: Get actual timestamp if needed
        QueueEvent(event);
        
        prev_value_ = current_value_;
    } else {
        current_delta_ = 0.0f;
        current_direction_ = EncoderDirection::NONE;
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

