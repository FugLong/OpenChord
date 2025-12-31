#pragma once

#include "../io/display_manager.h"
#include "../io/input_manager.h"
#include "../io/io_manager.h"
#include "../tracks/track_interface.h"
#include <cstdint>

namespace OpenChord {

// Forward declarations
class SystemBar;
class ContentArea;
class MenuManager;
class SettingsManager;
class IPluginWithSettings;
class OctaveUI;
class OctaveShift;

// Content render callback type
typedef void (*ContentRenderFunc)(DisplayManager* display);

/**
 * UI Manager - Centralized UI coordinator
 * 
 * Owns the display lifecycle and coordinates all UI rendering:
 * - System bar (battery, track name, context)
 * - Content area (routed to active renderer)
 * 
 * Other components request to render content, UI Manager handles the actual display.
 */
class UIManager {
public:
    enum class ContentType {
        NONE,        // No content (blank)
        MAIN_UI,     // Main UI (default chord display)
        DEBUG,       // Debug screen
        MENU,        // Menu system
        SETTINGS,    // Settings UI
        PLUGIN_UI,   // Plugin-defined custom UI
        OCTAVE_UI    // Octave adjustment overlay
    };
    
    UIManager();
    ~UIManager();
    
    // Initialization
    void Init(DisplayManager* display, InputManager* input_manager, IOManager* io_manager);
    
    // Main update loop (call from main loop)
    // Updates state but doesn't render - use Render() separately
    void Update();
    
    // Render the UI (call after Update() if rendering is needed)
    void Render();
    
    // Render just the system bar (for overlays like octave UI)
    void RenderSystemBar();
    
    // System bar control
    void SetTrack(Track* track);
    void SetTrackName(const char* name);  // Override track name display
    void SetContext(const char* context); // Set context indicator (e.g., "Menu", "Input", "DEBUG")
    
    // Octave shift system (must be set before using octave UI)
    void SetOctaveShift(OctaveShift* octave_shift);
    
    // Content area control
    void SetContentType(ContentType type);
    ContentType GetContentType() const { return content_type_; }
    
    // Register content renderers
    void SetMainUIRenderer(ContentRenderFunc render_func);
    void SetDebugRenderer(ContentRenderFunc render_func);
    void SetPluginRenderer(ContentRenderFunc render_func);
    void ClearPluginRenderer();
    
    // Menu and Settings access
    MenuManager* GetMenuManager() { return menu_manager_; }
    SettingsManager* GetSettingsManager() { return settings_manager_; }
    
    // Octave UI control (managed by UI Manager)
    void ActivateOctaveUI();
    void DeactivateOctaveUI();
    bool IsOctaveUIActive() const;
    void UpdateOctaveUI(float joystick_x, uint32_t current_time_ms);
    
    // Debug mode control (for external debug screen management)
    void SetDebugMode(bool enabled);
    bool IsDebugModeActive() const { return debug_mode_active_; }
    
    // Health check
    bool IsHealthy() const;
    
private:
    DisplayManager* display_;
    InputManager* input_manager_;
    IOManager* io_manager_;
    
    // UI components
    SystemBar* system_bar_;
    ContentArea* content_area_;
    MenuManager* menu_manager_;
    SettingsManager* settings_manager_;
    OctaveUI* octave_ui_;
    OctaveShift* octave_shift_;
    
    // Content management
    ContentType content_type_;
    ContentRenderFunc main_ui_render_func_;
    ContentRenderFunc debug_render_func_;
    ContentRenderFunc plugin_render_func_;
    Track* current_track_;
    
    // Rendering state
    uint32_t render_interval_ms_;
    uint32_t last_render_time_;
    bool needs_refresh_;
    bool debug_mode_active_;  // Track if debug mode is active
    
    void RenderContent();
    void RenderOctaveUI();
};

} // namespace OpenChord