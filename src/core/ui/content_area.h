#pragma once

#include "../io/display_manager.h"
#include <cstdint>

namespace OpenChord {

// Forward declarations
class MainUI;
class DebugScreen;

/**
 * Content Area - Manages the main content display area (128x48 pixels)
 * 
 * Routes rendering to appropriate content renderers:
 * - Normal mode → MainUI (plugin default view)
 * - Menu mode → Menu UI
 * - Settings mode → Settings UI
 * - Debug mode → DebugScreen
 * - Plugin-defined UI → Plugin custom renderer
 */
class ContentArea {
public:
    enum class Mode {
        NORMAL,      // Show plugin default view (MainUI)
        MENU,        // Show menu UI
        SETTINGS,    // Show settings UI
        DEBUG,       // Show debug screen
        PLUGIN_UI    // Plugin-defined custom UI
    };
    
    ContentArea();
    ~ContentArea();
    
    // Initialization
    void Init(DisplayManager* display);
    
    // Update and render
    void Update();
    void Render();
    
    // Mode management
    void SetMode(Mode mode) { mode_ = mode; }
    Mode GetMode() const { return mode_; }
    
    // Set content renderers
    void SetMainUI(MainUI* main_ui) { main_ui_ = main_ui; }
    void SetDebugScreen(DebugScreen* debug_screen) { debug_screen_ = debug_screen; }
    
    // Plugin UI control
    void SetPluginUI(void (*render_func)(DisplayManager*)) {
        plugin_render_func_ = render_func;
        if (render_func) {
            mode_ = Mode::PLUGIN_UI;
        }
    }
    void ClearPluginUI() {
        plugin_render_func_ = nullptr;
        if (mode_ == Mode::PLUGIN_UI) {
            mode_ = Mode::NORMAL;
        }
    }
    
    // Health check
    bool IsHealthy() const { return display_ != nullptr && display_->IsHealthy(); }
    
    // Content area bounds (for renderers to know available space)
    static constexpr uint8_t WIDTH = 128;
    static constexpr uint8_t HEIGHT = 48;  // 64 total - 8 top bar - 8 bottom bar (if used)
    static constexpr uint8_t OFFSET_Y = 10;  // Start below system bar (8px bar + 1px line + 1px spacing)
    
private:
    DisplayManager* display_;
    Mode mode_;
    
    // Content renderers
    MainUI* main_ui_;
    DebugScreen* debug_screen_;
    void (*plugin_render_func_)(DisplayManager*);
    
    void RenderNormalMode();
    void RenderDebugMode();
    void RenderPluginUI();
};

} // namespace OpenChord
