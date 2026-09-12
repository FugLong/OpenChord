#include "content_area.h"
#include "main_ui.h"
#include "debug_screen.h"

namespace OpenChord {

ContentArea::ContentArea()
    : display_(nullptr)
    , mode_(Mode::NORMAL)
    , main_ui_(nullptr)
    , debug_screen_(nullptr)
    , plugin_render_func_(nullptr)
{
}

ContentArea::~ContentArea() {
}

void ContentArea::Init(DisplayManager* display) {
    display_ = display;
    mode_ = Mode::NORMAL;
    main_ui_ = nullptr;
    debug_screen_ = nullptr;
    plugin_render_func_ = nullptr;
}

void ContentArea::Update() {
    // Note: Update is now handled by UIManager directly
    // This method is kept for future use when we refactor to separate Update() and Render()
}

void ContentArea::Render() {
    if (!IsHealthy()) return;
    
    // Render based on mode
    // Note: For now, MainUI and DebugScreen render to full screen
    // TODO: Update them to render only to content area (128x54, offset by 10 pixels)
    switch (mode_) {
        case Mode::NORMAL:
            RenderNormalMode();
            break;
            
        case Mode::DEBUG:
            RenderDebugMode();
            break;
            
        case Mode::MENU:
            // Menu rendering will be implemented in MenuManager
            // For now, content area stays empty
            break;
            
        case Mode::SETTINGS:
            // Settings rendering will be implemented in SettingsManager
            // For now, content area stays empty
            break;
            
        case Mode::PLUGIN_UI:
            RenderPluginUI();
            break;
    }
}

void ContentArea::RenderNormalMode() {
    // MainUI handles its own rendering via its Update() method
    // This is called from UIManager::Update(), not from here
    // TODO: Refactor to separate Update() and Render() methods
}

void ContentArea::RenderDebugMode() {
    // DebugScreen handles its own rendering via its Update() method
    // This is called from UIManager::Update(), not from here
    // TODO: Refactor to separate Update() and Render() methods
}

void ContentArea::RenderPluginUI() {
    if (plugin_render_func_ && display_) {
        plugin_render_func_(display_);
    }
}

} // namespace OpenChord
