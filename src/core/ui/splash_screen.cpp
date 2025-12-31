#include "splash_screen.h"
#include "daisy_seed.h"  // For System::GetNow()
#include <cstring>

namespace OpenChord {

SplashScreen::SplashScreen()
    : display_(nullptr)
    , show_splash_(true)
    , start_time_ms_(0)
{
}

SplashScreen::~SplashScreen() {
}

void SplashScreen::Init(DisplayManager* display) {
    display_ = display;
    show_splash_ = true;
    start_time_ms_ = daisy::System::GetNow();  // Record start time in milliseconds
}

void SplashScreen::Update() {
    if (!show_splash_) return;
    
    // Calculate elapsed time
    uint32_t current_time_ms = daisy::System::GetNow();
    uint32_t elapsed_ms = current_time_ms - start_time_ms_;
    
    // Hide splash once duration is reached
    if (elapsed_ms >= SPLASH_DURATION_MS) {
        show_splash_ = false;
    }
}

void SplashScreen::Render() {
    if (!display_ || !display_->IsHealthy()) return;
    
    auto* disp = display_->GetDisplay();
    if (!disp) return;
    
    // Clear display (black background)
    disp->Fill(false);
    
    // Render logo/text
    RenderLogo();
    
    // Update display
    disp->Update();
}

void SplashScreen::RenderLogo() {
    auto* disp = display_->GetDisplay();
    if (!disp) return;
    
    // Display "OpenChord" centered on screen
    // Use Font_11x18 for large text (18 pixels tall)
    // Screen is 128x64 pixels
    // Font_11x18 is 11 pixels wide per character
    // "OpenChord" = 9 characters = 99 pixels wide
    // Center: (128 - 99) / 2 = 14.5 ≈ 14 pixels from left
    
    const char* logo_text = "OpenChord";
    
    // Calculate centered position
    int text_width = strlen(logo_text) * 11;  // Font_11x18 is 11 pixels wide per char
    int x_pos = (128 - text_width) / 2;
    
    // Center vertically (64 pixels tall, font is 18 pixels tall)
    // Center: (64 - 18) / 2 = 23
    int y_pos = 23;
    
    disp->SetCursor(x_pos, y_pos);
    disp->WriteString(logo_text, Font_11x18, true);
}

} // namespace OpenChord
