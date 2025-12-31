#include "octave_ui.h"
#include <cstdio>
#include <cmath>
#include "daisy_seed.h"  // For Font_6x8, Font_11x18

namespace OpenChord {

OctaveUI::OctaveUI()
    : display_(nullptr)
    , octave_shift_(nullptr)
    , is_active_(false)
    , auto_hide_timeout_(0)
    , last_adjust_time_(0)
{
}

OctaveUI::~OctaveUI() {
}

void OctaveUI::Init(DisplayManager* display, OctaveShift* octave_shift) {
    display_ = display;
    octave_shift_ = octave_shift;
    is_active_ = false;
    auto_hide_timeout_ = 0;
    last_adjust_time_ = 0;
}

void OctaveUI::Activate() {
    is_active_ = true;
    last_adjust_time_ = 0;
}

void OctaveUI::Deactivate() {
    is_active_ = false;
    auto_hide_timeout_ = 0;
}

void OctaveUI::Update(float joystick_x, uint32_t current_time_ms) {
    if (!is_active_ || !octave_shift_) return;
    
    const float threshold = 0.3f;
    static uint32_t last_change_time = 0;
    
    // Check for octave adjustment
    if (std::abs(joystick_x) > threshold) {
        if (current_time_ms - last_change_time > 300) {  // Debounce every 300ms
            if (joystick_x > threshold) {
                octave_shift_->IncrementOctave();
            } else if (joystick_x < -threshold) {
                octave_shift_->DecrementOctave();
            }
            last_change_time = current_time_ms;
            last_adjust_time_ = current_time_ms;
        }
    }
    
    // Auto-hide after timeout (if no adjustments)
    if (last_adjust_time_ > 0 && (current_time_ms - last_adjust_time_) > AUTO_HIDE_TIMEOUT_MS) {
        Deactivate();
    }
}

void OctaveUI::Render() {
    if (!IsHealthy() || !is_active_) return;
    
    auto* disp = display_->GetDisplay();
    if (!disp || !octave_shift_) return;
    
    // Note: Display should already be cleared and system bar rendered by caller
    // We just render our content area (offset below system bar)
    
    // Render octave shift display
    RenderOctaveDisplay();
    
    // Update display (caller should do this, but do it here for safety)
    disp->Update();
}

void OctaveUI::RenderOctaveDisplay() {
    auto* disp = display_->GetDisplay();
    if (!disp || !octave_shift_) return;
    
    int shift = octave_shift_->GetOctaveShift();
    
    // Content area starts at y=10 (below system bar)
    // Render "Octave" label and value in large font, centered in content area
    
    const char* label = "Octave";
    int label_width = 6 * 6;  // Font_6x8, 6 chars * 6 pixels
    int label_x = (128 - label_width) / 2;
    
    // Value display
    char value_text[8];
    if (shift == 0) {
        snprintf(value_text, sizeof(value_text), "0");
    } else if (shift > 0) {
        snprintf(value_text, sizeof(value_text), "+%d", shift);
    } else {
        snprintf(value_text, sizeof(value_text), "%d", shift);
    }
    
    // Render label (small font, offset below system bar)
    disp->SetCursor(label_x, 20);
    disp->WriteString(label, Font_6x8, true);
    
    // Render value (large font, center in content area)
    int value_width = (shift == 0 ? 1 : (shift > 0 ? 2 : 2)) * 11;  // Font_11x18 width
    int value_x = (128 - value_width) / 2;
    disp->SetCursor(value_x, 35);
    disp->WriteString(value_text, Font_11x18, true);
    
    // Note: disp->Update() is called in Render(), not here
}

} // namespace OpenChord

