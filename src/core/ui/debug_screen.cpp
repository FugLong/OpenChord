#include "debug_screen.h"
#include "../io/button_input_handler.h"
#include <algorithm>

namespace OpenChord {

DebugScreen::DebugScreen()
    : display_(nullptr)
    , input_manager_(nullptr)
    , current_view_index_(-1)
    , enabled_(true)
    , render_interval_ms_(100)  // 10 FPS default
    , last_render_time_(0)
    , prev_input_pressed_(false)
    , prev_record_pressed_(false)
    , combo_hold_time_(0)
    , already_toggled_this_cycle_(false)
{
}

DebugScreen::~DebugScreen() {
    views_.clear();
}

void DebugScreen::Init(DisplayManager* display, InputManager* input_manager) {
    display_ = display;
    input_manager_ = input_manager;
    
    if (views_.empty()) {
        current_view_index_ = -1;
    } else {
        current_view_index_ = 0;
    }
    
    last_render_time_ = 0;
    prev_input_pressed_ = false;
    prev_record_pressed_ = false;
    combo_hold_time_ = 0;
    already_toggled_this_cycle_ = false;
}

void DebugScreen::AddView(const char* name, DebugRenderFunc render_func) {
    if (name && render_func) {
        views_.push_back(DebugView(name, render_func));
        
        // If this is the first view, set it as current
        if (current_view_index_ == -1 && views_.size() == 1) {
            current_view_index_ = 0;
        }
    }
}

void DebugScreen::Update() {
    // Always handle toggle combo, even when disabled (allows toggling on)
    HandleToggleCombo();
    
    // Only process navigation and rendering if enabled
    if (!enabled_ || !display_ || !display_->IsHealthy()) {
        return;
    }
    
    if (views_.empty() || current_view_index_ < 0 || 
        current_view_index_ >= static_cast<int>(views_.size())) {
        return;
    }
    
    // Handle navigation
    HandleNavigation();
    
    // Render at configured interval
    if (last_render_time_ >= render_interval_ms_) {
        RenderCurrentView();
        last_render_time_ = 0;
    } else {
        last_render_time_++;
    }
}

void DebugScreen::NextView() {
    if (views_.empty()) return;
    current_view_index_ = (current_view_index_ + 1) % static_cast<int>(views_.size());
}

void DebugScreen::PreviousView() {
    if (views_.empty()) return;
    current_view_index_--;
    if (current_view_index_ < 0) {
        current_view_index_ = static_cast<int>(views_.size()) - 1;
    }
}

void DebugScreen::SetView(int index) {
    if (views_.empty() || index < 0 || index >= static_cast<int>(views_.size())) {
        return;
    }
    current_view_index_ = index;
}

const char* DebugScreen::GetCurrentViewName() const {
    if (current_view_index_ >= 0 && current_view_index_ < static_cast<int>(views_.size())) {
        return views_[current_view_index_].name;
    }
    return nullptr;
}

void DebugScreen::Render(DisplayManager* display) {
    if (!display || !display->IsHealthy()) {
        RenderCurrentView();  // Fallback to internal render if display not provided
        return;
    }
    
    // Use provided display instead of stored one
    RenderCurrentViewInternal(display);
}

void DebugScreen::RenderCurrentView() {
    if (!display_ || !display_->IsHealthy() || views_.empty()) {
        return;
    }
    
    RenderCurrentViewInternal(display_);
}

void DebugScreen::RenderCurrentViewInternal(DisplayManager* display) {
    if (!display || !display->IsHealthy() || views_.empty()) {
        return;
    }
    
    if (current_view_index_ < 0 || current_view_index_ >= static_cast<int>(views_.size())) {
        return;
    }
    
    const DebugView& view = views_[current_view_index_];
    if (view.render) {
        // Note: Display clearing and Update() are now handled by UIManager
        // Just call the render function - it should render to content area (offset by 10 pixels)
        view.render(display);
    }
}

void DebugScreen::HandleToggleCombo() {
    if (!input_manager_) return;
    
    ButtonInputHandler& buttons = input_manager_->GetButtons();
    
    bool input_pressed = buttons.IsSystemButtonPressed(SystemButton::INPUT);
    bool record_pressed = buttons.IsSystemButtonPressed(SystemButton::RECORD);
    
    // Check if both buttons are pressed together
    bool both_pressed = input_pressed && record_pressed;
    bool both_pressed_prev = prev_input_pressed_ && prev_record_pressed_;
    
    // If both just pressed, start timing
    if (both_pressed && !both_pressed_prev) {
        combo_hold_time_ = 0;
        already_toggled_this_cycle_ = false;
    }
    
    // If both are held, increment timer
    if (both_pressed) {
        combo_hold_time_++;
        
        // Toggle after 500ms (0.5 seconds) or 1000ms (1 second) hold
        if (combo_hold_time_ >= COMBO_HOLD_THRESHOLD_MS && !already_toggled_this_cycle_) {
            enabled_ = !enabled_;
            already_toggled_this_cycle_ = true;
        }
    } else {
        // Not both pressed - reset
        combo_hold_time_ = 0;
        already_toggled_this_cycle_ = false;
    }
    
    prev_input_pressed_ = input_pressed;
    prev_record_pressed_ = record_pressed;
}

void DebugScreen::HandleNavigation() {
    if (!input_manager_ || !enabled_) return;
    
    ButtonInputHandler& buttons = input_manager_->GetButtons();
    
    // INPUT button (left/up) = Previous view (only if RECORD not also pressed)
    if (buttons.WasSystemButtonPressed(SystemButton::INPUT) && 
        !buttons.IsSystemButtonPressed(SystemButton::RECORD)) {
        PreviousView();
    }
    
    // RECORD button (right/down) = Next view (only if INPUT not also pressed)
    if (buttons.WasSystemButtonPressed(SystemButton::RECORD) && 
        !buttons.IsSystemButtonPressed(SystemButton::INPUT)) {
        NextView();
    }
}

} // namespace OpenChord