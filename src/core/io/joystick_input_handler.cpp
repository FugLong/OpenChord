#include "joystick_input_handler.h"
#include <cstring>
#include <cmath>

namespace OpenChord {

JoystickInputHandler::JoystickInputHandler()
    : analog_manager_(nullptr),
      mode_(JoystickMode::NAVIGATION),
      dead_zone_(0.05f),
      movement_threshold_(0.01f),
      current_x_(0.0f),
      current_y_(0.0f),
      prev_x_(0.0f),
      prev_y_(0.0f),
      event_queue_head_(0),
      event_queue_tail_(0),
      event_queue_count_(0) {
    
    memset(event_queue_, 0, sizeof(event_queue_));
}

JoystickInputHandler::~JoystickInputHandler() {
    analog_manager_ = nullptr;
}

void JoystickInputHandler::Init(::AnalogManager* analog_manager) {
    analog_manager_ = analog_manager;
    mode_ = JoystickMode::NAVIGATION;
    dead_zone_ = 0.05f;
    movement_threshold_ = 0.01f;
    
    current_x_ = 0.0f;
    current_y_ = 0.0f;
    prev_x_ = 0.0f;
    prev_y_ = 0.0f;
    
    event_queue_head_ = 0;
    event_queue_tail_ = 0;
    event_queue_count_ = 0;
}

void JoystickInputHandler::Update() {
    if (!analog_manager_) return;
    
    ProcessJoystick();
}

void JoystickInputHandler::ProcessJoystick() {
    if (!analog_manager_) return;
    
    // Get raw joystick values (0.0 to 1.0 from AnalogManager)
    float raw_x = analog_manager_->GetJoystickX();
    float raw_y = analog_manager_->GetJoystickY();
    
    // Convert to centered coordinates (-1.0 to 1.0, with 0.0 at center)
    // AnalogManager returns normalized values, we need to center them
    // Assuming center is at 0.5 (typical for joysticks)
    current_x_ = (raw_x - 0.5f) * 2.0f;
    current_y_ = (raw_y - 0.5f) * 2.0f;
    
    // Apply dead zone
    float magnitude = sqrtf(current_x_ * current_x_ + current_y_ * current_y_);
    if (magnitude < dead_zone_) {
        current_x_ = 0.0f;
        current_y_ = 0.0f;
    } else {
        // Scale to remove dead zone
        float scale = (magnitude - dead_zone_) / (1.0f - dead_zone_);
        current_x_ = (current_x_ / magnitude) * scale;
        current_y_ = (current_y_ / magnitude) * scale;
    }
    
    // Clamp to -1.0 to 1.0 range
    if (current_x_ > 1.0f) current_x_ = 1.0f;
    if (current_x_ < -1.0f) current_x_ = -1.0f;
    if (current_y_ > 1.0f) current_y_ = 1.0f;
    if (current_y_ < -1.0f) current_y_ = -1.0f;
    
    // Check for movement events
    float delta_x = current_x_ - prev_x_;
    float delta_y = current_y_ - prev_y_;
    float movement = sqrtf(delta_x * delta_x + delta_y * delta_y);
    
    if (movement > movement_threshold_) {
        JoystickEvent event;
        event.type = JoystickEventType::MOVED;
        event.x = current_x_;
        event.y = current_y_;
        event.delta_x = delta_x;
        event.delta_y = delta_y;
        event.timestamp = 0; // TODO: Get actual timestamp if needed
        QueueEvent(event);
    }
    
    // Check for centering
    bool was_centered = (fabsf(prev_x_) < dead_zone_ && fabsf(prev_y_) < dead_zone_);
    bool is_centered = (fabsf(current_x_) < dead_zone_ && fabsf(current_y_) < dead_zone_);
    if (!was_centered && is_centered) {
        JoystickEvent event;
        event.type = JoystickEventType::CENTERED;
        event.x = current_x_;
        event.y = current_y_;
        event.delta_x = delta_x;
        event.delta_y = delta_y;
        event.timestamp = 0;
        QueueEvent(event);
    }
    
    // Check for edge reached
    bool at_edge = (fabsf(current_x_) > 0.95f || fabsf(current_y_) > 0.95f);
    bool was_at_edge = (fabsf(prev_x_) > 0.95f || fabsf(prev_y_) > 0.95f);
    if (!was_at_edge && at_edge) {
        JoystickEvent event;
        event.type = JoystickEventType::EDGE_REACHED;
        event.x = current_x_;
        event.y = current_y_;
        event.delta_x = delta_x;
        event.delta_y = delta_y;
        event.timestamp = 0;
        QueueEvent(event);
    }
    
    prev_x_ = current_x_;
    prev_y_ = current_y_;
}

void JoystickInputHandler::QueueEvent(const JoystickEvent& event) {
    if (event_queue_count_ >= MAX_EVENTS) {
        // Queue full, drop oldest event
        event_queue_head_ = (event_queue_head_ + 1) % MAX_EVENTS;
        event_queue_count_--;
    }
    
    event_queue_[event_queue_tail_] = event;
    event_queue_tail_ = (event_queue_tail_ + 1) % MAX_EVENTS;
    event_queue_count_++;
}

bool JoystickInputHandler::PollEvent(JoystickEvent& event) {
    if (event_queue_count_ == 0) {
        return false;
    }
    
    event = event_queue_[event_queue_head_];
    event_queue_head_ = (event_queue_head_ + 1) % MAX_EVENTS;
    event_queue_count_--;
    return true;
}

float JoystickInputHandler::GetX() const {
    return current_x_;
}

float JoystickInputHandler::GetY() const {
    return current_y_;
}

void JoystickInputHandler::GetPosition(float* x, float* y) const {
    if (x) *x = current_x_;
    if (y) *y = current_y_;
}

float JoystickInputHandler::GetDeltaX() const {
    return current_x_ - prev_x_;
}

float JoystickInputHandler::GetDeltaY() const {
    return current_y_ - prev_y_;
}

JoystickDirection JoystickInputHandler::GetDirection() const {
    return CalculateDirection(current_x_, current_y_);
}

bool JoystickInputHandler::IsCentered() const {
    return (fabsf(current_x_) < dead_zone_ && fabsf(current_y_) < dead_zone_);
}

bool JoystickInputHandler::IsAtEdge() const {
    return (fabsf(current_x_) > 0.95f || fabsf(current_y_) > 0.95f);
}

bool JoystickInputHandler::IsPushedUp(float threshold) const {
    return current_y_ > threshold;
}

bool JoystickInputHandler::IsPushedDown(float threshold) const {
    return current_y_ < -threshold;
}

bool JoystickInputHandler::IsPushedLeft(float threshold) const {
    return current_x_ < -threshold;
}

bool JoystickInputHandler::IsPushedRight(float threshold) const {
    return current_x_ > threshold;
}

JoystickDirection JoystickInputHandler::CalculateDirection(float x, float y) const {
    if (fabsf(x) < dead_zone_ && fabsf(y) < dead_zone_) {
        return JoystickDirection::CENTER;
    }
    
    // Calculate angle and determine primary direction
    float angle = atan2f(y, x) * 180.0f / 3.14159265f;
    
    // Normalize angle to 0-360
    if (angle < 0) angle += 360.0f;
    
    // Determine direction based on angle
    if (angle >= 337.5f || angle < 22.5f) return JoystickDirection::RIGHT;
    if (angle >= 22.5f && angle < 67.5f) return JoystickDirection::UP_RIGHT;
    if (angle >= 67.5f && angle < 112.5f) return JoystickDirection::UP;
    if (angle >= 112.5f && angle < 157.5f) return JoystickDirection::UP_LEFT;
    if (angle >= 157.5f && angle < 202.5f) return JoystickDirection::LEFT;
    if (angle >= 202.5f && angle < 247.5f) return JoystickDirection::DOWN_LEFT;
    if (angle >= 247.5f && angle < 292.5f) return JoystickDirection::DOWN;
    if (angle >= 292.5f && angle < 337.5f) return JoystickDirection::DOWN_RIGHT;
    
    return JoystickDirection::CENTER;
}

bool JoystickInputHandler::HasMoved(float threshold) const {
    float delta_x = current_x_ - prev_x_;
    float delta_y = current_y_ - prev_y_;
    float movement = sqrtf(delta_x * delta_x + delta_y * delta_y);
    return movement > threshold;
}

void JoystickInputHandler::SetDeadZone(float dead_zone) {
    dead_zone_ = dead_zone;
    if (dead_zone_ < 0.0f) dead_zone_ = 0.0f;
    if (dead_zone_ > 0.5f) dead_zone_ = 0.5f;
}

void JoystickInputHandler::SetMovementThreshold(float threshold) {
    movement_threshold_ = threshold;
    if (movement_threshold_ < 0.0f) movement_threshold_ = 0.0f;
}

} // namespace OpenChord

