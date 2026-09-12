#pragma once

#include "../io/display_manager.h"
#include <cstdint>

namespace OpenChord {

/**
 * Splash Screen - Boot screen shown on startup
 * 
 * Displays "OpenChord" logo/text in large font
 */
class SplashScreen {
public:
    SplashScreen();
    ~SplashScreen();
    
    // Initialization
    void Init(DisplayManager* display);
    
    // Render splash screen
    void Render();
    
    // Check if splash screen should be shown (call before showing main UI)
    bool ShouldShow() const { return show_splash_; }
    
    // Update (call from main loop to track display time)
    void Update();
    
    // Force hide splash screen
    void Hide() { show_splash_ = false; }
    
private:
    DisplayManager* display_;
    bool show_splash_;
    uint32_t start_time_ms_;  // System time when splash started (in milliseconds)
    
    static constexpr uint32_t SPLASH_DURATION_MS = 1500;  // Show splash for 1.5 seconds
    
    void RenderLogo();
};

} // namespace OpenChord
