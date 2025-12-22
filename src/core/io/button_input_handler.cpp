#include "button_input_handler.h"
#include <cstring>

namespace OpenChord {

ButtonInputHandler::ButtonInputHandler()
    : digital_manager_(nullptr),
      input_mode_(InputMode::MIDI_NOTES),
      hold_threshold_ms_(500),
      event_queue_head_(0),
      event_queue_tail_(0),
      event_queue_count_(0) {
    
    memset(prev_musical_states_, 0, sizeof(prev_musical_states_));
    memset(prev_system_states_, 0, sizeof(prev_system_states_));
    memset(event_queue_, 0, sizeof(event_queue_));
}

ButtonInputHandler::~ButtonInputHandler() {
    digital_manager_ = nullptr;
}

void ButtonInputHandler::Init(::DigitalManager* digital_manager) {
    digital_manager_ = digital_manager;
    input_mode_ = InputMode::MIDI_NOTES;
    hold_threshold_ms_ = 500;
    
    // Reset state
    memset(prev_musical_states_, 0, sizeof(prev_musical_states_));
    memset(prev_system_states_, 0, sizeof(prev_system_states_));
    event_queue_head_ = 0;
    event_queue_tail_ = 0;
    event_queue_count_ = 0;
}

void ButtonInputHandler::Update() {
    if (!digital_manager_) return;
    
    // Process musical buttons and system buttons
    ProcessMusicalButtons();
    ProcessSystemButtons();
}

void ButtonInputHandler::ProcessMusicalButtons() {
    if (!digital_manager_) return;
    
    for (int i = 0; i < static_cast<int>(MusicalButton::COUNT); i++) {
        MusicalButton button = static_cast<MusicalButton>(i);
        int row, col;
        MusicalButtonToRowCol(button, row, col);
        
        bool current_pressed = digital_manager_->IsKeyPressed(row, col);
        bool was_pressed = digital_manager_->WasKeyPressed(row, col);
        bool prev_pressed = prev_musical_states_[i];
        
        // Detect press event
        if (was_pressed || (!prev_pressed && current_pressed)) {
            ButtonEvent event;
            event.type = ButtonEventType::PRESSED;
            event.timestamp = 0; // TODO: Get actual timestamp if needed
            event.musical_button = button;
            event.is_musical = true;
            QueueEvent(event);
        }
        
        // Detect release event
        if (prev_pressed && !current_pressed) {
            ButtonEvent event;
            event.type = ButtonEventType::RELEASED;
            event.timestamp = 0;
            event.musical_button = button;
            event.is_musical = true;
            QueueEvent(event);
        }
        
        // Detect hold event (after threshold)
        if (current_pressed) {
            uint32_t hold_time = digital_manager_->GetKeyHoldTime(row, col);
            if (hold_time > hold_threshold_ms_ && !prev_musical_states_[i]) {
                // Only trigger once when threshold is crossed
                ButtonEvent event;
                event.type = ButtonEventType::HELD;
                event.timestamp = 0;
                event.musical_button = button;
                event.is_musical = true;
                QueueEvent(event);
            }
        }
        
        prev_musical_states_[i] = current_pressed;
    }
}

void ButtonInputHandler::ProcessSystemButtons() {
    if (!digital_manager_) return;
    
    for (int i = 0; i < static_cast<int>(SystemButton::COUNT); i++) {
        SystemButton button = static_cast<SystemButton>(i);
        int row, col;
        SystemButtonToRowCol(button, row, col);
        
        bool current_pressed = digital_manager_->IsKeyPressed(row, col);
        bool was_pressed = digital_manager_->WasKeyPressed(row, col);
        bool prev_pressed = prev_system_states_[i];
        
        // Detect press event
        if (was_pressed || (!prev_pressed && current_pressed)) {
            ButtonEvent event;
            event.type = ButtonEventType::PRESSED;
            event.timestamp = 0;
            event.system_button = button;
            event.is_musical = false;
            QueueEvent(event);
        }
        
        // Detect release event
        if (prev_pressed && !current_pressed) {
            ButtonEvent event;
            event.type = ButtonEventType::RELEASED;
            event.timestamp = 0;
            event.system_button = button;
            event.is_musical = false;
            QueueEvent(event);
        }
        
        // Detect hold event (after threshold)
        if (current_pressed) {
            uint32_t hold_time = digital_manager_->GetKeyHoldTime(row, col);
            if (hold_time > hold_threshold_ms_ && !prev_system_states_[i]) {
                ButtonEvent event;
                event.type = ButtonEventType::HELD;
                event.timestamp = 0;
                event.system_button = button;
                event.is_musical = false;
                QueueEvent(event);
            }
        }
        
        prev_system_states_[i] = current_pressed;
    }
}

void ButtonInputHandler::QueueEvent(const ButtonEvent& event) {
    if (event_queue_count_ >= MAX_EVENTS) {
        // Queue full, drop oldest event (simple FIFO)
        event_queue_head_ = (event_queue_head_ + 1) % MAX_EVENTS;
        event_queue_count_--;
    }
    
    event_queue_[event_queue_tail_] = event;
    event_queue_tail_ = (event_queue_tail_ + 1) % MAX_EVENTS;
    event_queue_count_++;
}

bool ButtonInputHandler::PollEvent(ButtonEvent& event) {
    if (event_queue_count_ == 0) {
        return false;
    }
    
    event = event_queue_[event_queue_head_];
    event_queue_head_ = (event_queue_head_ + 1) % MAX_EVENTS;
    event_queue_count_--;
    return true;
}

void ButtonInputHandler::MusicalButtonToRowCol(MusicalButton button, int& row, int& col) const {
    int idx = static_cast<int>(button);
    
    if (idx < 4) {
        // Bottom row (white keys) - Row 0
        row = 0;
        col = idx;
    } else {
        // Middle row (black keys) - Row 1
        row = 1;
        col = idx - 4;
    }
}

void ButtonInputHandler::SystemButtonToRowCol(SystemButton button, int& row, int& col) const {
    // All system buttons are on Row 2 (top row)
    row = 2;
    col = static_cast<int>(button);
}

bool ButtonInputHandler::IsMusicalButtonPressed(MusicalButton button) const {
    if (!digital_manager_) return false;
    
    int row, col;
    MusicalButtonToRowCol(button, row, col);
    return digital_manager_->IsKeyPressed(row, col);
}

bool ButtonInputHandler::WasMusicalButtonPressed(MusicalButton button) const {
    if (!digital_manager_) return false;
    
    int row, col;
    MusicalButtonToRowCol(button, row, col);
    return digital_manager_->WasKeyPressed(row, col);
}

bool ButtonInputHandler::IsMusicalButtonHeld(MusicalButton button) const {
    if (!digital_manager_) return false;
    
    int row, col;
    MusicalButtonToRowCol(button, row, col);
    return digital_manager_->IsKeyHeld(row, col);
}

uint32_t ButtonInputHandler::GetMusicalButtonHoldTime(MusicalButton button) const {
    if (!digital_manager_) return 0;
    
    int row, col;
    MusicalButtonToRowCol(button, row, col);
    return digital_manager_->GetKeyHoldTime(row, col);
}

bool ButtonInputHandler::IsSystemButtonPressed(SystemButton button) const {
    if (!digital_manager_) return false;
    
    int row, col;
    SystemButtonToRowCol(button, row, col);
    return digital_manager_->IsKeyPressed(row, col);
}

bool ButtonInputHandler::WasSystemButtonPressed(SystemButton button) const {
    if (!digital_manager_) return false;
    
    int row, col;
    SystemButtonToRowCol(button, row, col);
    return digital_manager_->WasKeyPressed(row, col);
}

bool ButtonInputHandler::IsSystemButtonHeld(SystemButton button) const {
    if (!digital_manager_) return false;
    
    int row, col;
    SystemButtonToRowCol(button, row, col);
    return digital_manager_->IsKeyHeld(row, col);
}

uint32_t ButtonInputHandler::GetSystemButtonHoldTime(SystemButton button) const {
    if (!digital_manager_) return 0;
    
    int row, col;
    SystemButtonToRowCol(button, row, col);
    return digital_manager_->GetKeyHoldTime(row, col);
}

bool ButtonInputHandler::IsRawButtonPressed(int row, int col) const {
    if (!digital_manager_) return false;
    return digital_manager_->IsKeyPressed(row, col);
}

bool ButtonInputHandler::WasRawButtonPressed(int row, int col) const {
    if (!digital_manager_) return false;
    return digital_manager_->WasKeyPressed(row, col);
}

void ButtonInputHandler::SetHoldThreshold(uint32_t ms) {
    hold_threshold_ms_ = ms;
    if (digital_manager_) {
        digital_manager_->SetButtonHoldThreshold(ms);
    }
}

} // namespace OpenChord

