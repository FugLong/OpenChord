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

void DebugScreen::RenderCurrentView() {
    if (!display_ || !display_->IsHealthy() || views_.empty()) {
        return;
    }
    
    if (current_view_index_ < 0 || current_view_index_ >= static_cast<int>(views_.size())) {
        return;
    }
    
    const DebugView& view = views_[current_view_index_];
    if (view.render) {
        // Clear display first
        daisy::OledDisplay<daisy::SSD130x4WireSpi128x64Driver>* disp = display_->GetDisplay();
        if (disp) {
            disp->Fill(false);
        }
        
        // Call render function
        view.render(display_);
    }
}

void DebugScreen::HandleNavigation() {
    if (!input_manager_) return;
    
    ButtonInputHandler& buttons = input_manager_->GetButtons();
    
    // INPUT button (left/up) = Previous view
    if (buttons.WasSystemButtonPressed(SystemButton::INPUT)) {
        PreviousView();
    }
    
    // RECORD button (right/down) = Next view
    if (buttons.WasSystemButtonPressed(SystemButton::RECORD)) {
        NextView();
    }
}

} // namespace OpenChord