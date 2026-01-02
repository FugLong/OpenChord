#include "ui_manager.h"
#include "system_bar.h"
#include "content_area.h"
#include "menu_manager.h"
#include "settings_manager.h"
#include "octave_ui.h"
#include "../midi/octave_shift.h"
#include "../io/power_manager.h"
#include <new>

namespace OpenChord {

UIManager::UIManager()
    : display_(nullptr)
    , input_manager_(nullptr)
    , io_manager_(nullptr)
    , system_bar_(nullptr)
    , content_area_(nullptr)
    , menu_manager_(nullptr)
    , settings_manager_(nullptr)
    , octave_ui_(nullptr)
    , octave_shift_(nullptr)
    , content_type_(ContentType::NONE)
    , main_ui_render_func_(nullptr)
    , debug_render_func_(nullptr)
    , plugin_render_func_(nullptr)
    , current_track_(nullptr)
    , render_interval_ms_(100)  // 10 FPS default
    , last_render_time_(0)
    , needs_refresh_(true)
    , debug_mode_active_(false)
    , power_mgr_(nullptr)
{
    // Allocate UI components
        system_bar_ = new (std::nothrow) SystemBar();
        content_area_ = new (std::nothrow) ContentArea();
        menu_manager_ = new (std::nothrow) MenuManager();
        settings_manager_ = new (std::nothrow) SettingsManager();
        octave_ui_ = new (std::nothrow) OctaveUI();
}

UIManager::~UIManager() {
    if (system_bar_) {
        delete system_bar_;
        system_bar_ = nullptr;
    }
    if (content_area_) {
        delete content_area_;
        content_area_ = nullptr;
    }
    if (menu_manager_) {
        delete menu_manager_;
        menu_manager_ = nullptr;
    }
    if (settings_manager_) {
        delete settings_manager_;
        settings_manager_ = nullptr;
    }
    if (octave_ui_) {
        delete octave_ui_;
        octave_ui_ = nullptr;
    }
}

void UIManager::Init(DisplayManager* display, InputManager* input_manager, IOManager* io_manager) {
    display_ = display;
    input_manager_ = input_manager;
    io_manager_ = io_manager;
    
    // Initialize system bar
    if (system_bar_) {
        system_bar_->Init(display, io_manager);
    }
    
        // Initialize content area
    if (content_area_) {
        content_area_->Init(display);
    }
    
    // Initialize menu and settings managers
    if (menu_manager_) {
        menu_manager_->Init(display, input_manager);
        menu_manager_->SetTrack(current_track_);
    }
    if (settings_manager_) {
        settings_manager_->Init(display);
    }
    
    // Octave UI will be initialized when octave shift is set
    
    content_type_ = ContentType::NONE;
    current_track_ = nullptr;
    main_ui_render_func_ = nullptr;
    debug_render_func_ = nullptr;
    plugin_render_func_ = nullptr;
    
    last_render_time_ = 0;
    needs_refresh_ = true;
}

void UIManager::SetTrack(Track* track) {
    current_track_ = track;
    if (system_bar_) {
        system_bar_->SetTrack(track);
    }
    if (menu_manager_) {
        menu_manager_->SetTrack(track);
    }
}

void UIManager::SetTrackName(const char* name) {
    if (system_bar_) {
        system_bar_->SetTrackName(name);
    }
}

void UIManager::SetContext(const char* context) {
    if (system_bar_) {
        system_bar_->SetContext(context);
    }
}

void UIManager::SetContentType(ContentType type) {
    content_type_ = type;
    needs_refresh_ = true;
}

void UIManager::SetMainUIRenderer(ContentRenderFunc render_func) {
    main_ui_render_func_ = render_func;
}

void UIManager::SetDebugRenderer(ContentRenderFunc render_func) {
    debug_render_func_ = render_func;
}

void UIManager::SetPluginRenderer(ContentRenderFunc render_func) {
    plugin_render_func_ = render_func;
    if (render_func) {
        content_type_ = ContentType::PLUGIN_UI;
    }
}

void UIManager::ClearPluginRenderer() {
    plugin_render_func_ = nullptr;
    if (content_type_ == ContentType::PLUGIN_UI) {
        content_type_ = ContentType::NONE;
    }
}

bool UIManager::IsHealthy() const {
    return display_ != nullptr && 
           display_->IsHealthy() &&
           system_bar_ != nullptr &&
           content_area_ != nullptr;
}

void UIManager::Update() {
    if (!IsHealthy()) return;
    
        // Update system bar (battery, etc.)
    if (system_bar_) {
        system_bar_->Update();
    }
    
    // Update menu manager (handles input and state)
    if (menu_manager_) {
        menu_manager_->Update();
        // Check if menu manager needs a refresh (e.g., plugin state changed)
        // Note: MenuManager doesn't expose needs_refresh_ directly, but we can check via Render()
        // Actually, we'll handle refresh via the normal render cycle
    }
    
    // Priority order: Octave UI > Debug Mode > Menu/Settings > Main UI
    
    // Check if octave UI is active (takes highest priority)
    if (octave_ui_ && octave_ui_->IsActive()) {
        // Update content type to octave UI
        if (content_type_ != ContentType::OCTAVE_UI) {
            content_type_ = ContentType::OCTAVE_UI;
        }
    } else if (debug_mode_active_) {
        // Debug mode takes priority over menu/settings
        if (content_type_ != ContentType::DEBUG) {
            // Close any open menus when entering debug mode
            if (menu_manager_ && menu_manager_->IsOpen()) {
                menu_manager_->CloseMenu();
            }
            if (settings_manager_ && settings_manager_->GetPlugin()) {
                settings_manager_->SetPlugin(nullptr);
            }
            SetContentType(ContentType::DEBUG);
        }
    } else {
        // Update content type based on menu/settings state
        if (menu_manager_ && menu_manager_->IsOpen()) {
            if (settings_manager_ && settings_manager_->GetPlugin()) {
                // In settings mode
                if (content_type_ != ContentType::SETTINGS) {
                    SetContentType(ContentType::SETTINGS);
                }
            } else {
                // In menu mode
                if (content_type_ != ContentType::MENU) {
                    SetContentType(ContentType::MENU);
                }
            }
        } else {
            // No menu open - return to appropriate content
            if (content_type_ == ContentType::OCTAVE_UI || content_type_ == ContentType::DEBUG) {
                // Was in overlay mode, return to main UI
                SetContentType(ContentType::MAIN_UI);
            } else if (content_type_ != ContentType::MAIN_UI) {
                // Make sure we're on main UI if nothing else is active
                SetContentType(ContentType::MAIN_UI);
            }
        }
    }
    
    // Update render interval based on power mode (adaptive refresh)
    if (power_mgr_) {
        render_interval_ms_ = power_mgr_->GetDisplayInterval();
    }
    
    // Render periodically (UI Manager owns all rendering)
    if (last_render_time_ >= render_interval_ms_ || needs_refresh_) {
        Render();
        last_render_time_ = 0;
        needs_refresh_ = false;
    } else {
        last_render_time_++;
    }
}

void UIManager::Render() {
    if (!IsHealthy()) return;
    
    auto* disp = display_->GetDisplay();
    if (!disp) return;
    
    // Clear entire display
    disp->Fill(false);
    
    // Priority order: Octave UI > Debug Mode > Menu/Settings > Main UI
    
    // Octave UI takes priority - render it if active
    if (content_type_ == ContentType::OCTAVE_UI && octave_ui_ && octave_ui_->IsActive()) {
        RenderOctaveUI();
        return;  // Octave UI handles its own display update
    }
    
    // Debug mode takes priority over menu/settings
    if (content_type_ == ContentType::DEBUG) {
        // Render debug content only (no menu overlay)
        RenderContent();
    } else {
        // Render content area first
        RenderContent();
        
        // Render menu or settings if active (only if not in debug mode)
        if (menu_manager_ && menu_manager_->IsOpen()) {
            if (settings_manager_ && settings_manager_->GetPlugin()) {
                settings_manager_->Render();
            } else {
                menu_manager_->Render();
            }
        }
    }
    
    // Render system bar on top (always visible)
    if (system_bar_) {
        system_bar_->Render();
    }
    
    // Update display (once at the end)
    disp->Update();
}

void UIManager::RenderOctaveUI() {
    if (!octave_ui_ || !octave_ui_->IsActive()) return;
    
    // Render system bar first (always visible)
    if (system_bar_) {
        system_bar_->Render();
    }
    
    // Render octave UI content (handles its own display update)
    octave_ui_->Render();
}

void UIManager::RenderSystemBar() {
    if (!IsHealthy() || !system_bar_) return;
    system_bar_->Render();
}

void UIManager::SetOctaveShift(OctaveShift* octave_shift) {
    octave_shift_ = octave_shift;
    
    // Initialize octave UI when octave shift is provided
    if (octave_ui_ && octave_shift_ && display_) {
        octave_ui_->Init(display_, octave_shift_);
    }
}

void UIManager::ActivateOctaveUI() {
    if (octave_ui_) {
        octave_ui_->Activate();
        needs_refresh_ = true;  // Force immediate render
    }
}

void UIManager::DeactivateOctaveUI() {
    if (octave_ui_) {
        octave_ui_->Deactivate();
        needs_refresh_ = true;  // Force immediate render
    }
}

bool UIManager::IsOctaveUIActive() const {
    return octave_ui_ && octave_ui_->IsActive();
}

void UIManager::UpdateOctaveUI(float joystick_x, uint32_t current_time_ms) {
    if (octave_ui_ && octave_ui_->IsActive()) {
        octave_ui_->Update(joystick_x, current_time_ms);
        needs_refresh_ = true;  // Request render after update
    }
}

void UIManager::SetPowerManager(PowerManager* power_mgr) {
    power_mgr_ = power_mgr;
}

void UIManager::SetDebugMode(bool enabled) {
    debug_mode_active_ = enabled;
    needs_refresh_ = true;  // Force immediate render update
    
    if (enabled) {
        // Close any open menus when entering debug mode
        if (menu_manager_ && menu_manager_->IsOpen()) {
            menu_manager_->CloseMenu();
        }
        if (settings_manager_ && settings_manager_->GetPlugin()) {
            settings_manager_->SetPlugin(nullptr);
        }
    }
}

void UIManager::RenderContent() {
    if (!IsHealthy()) return;
    
    // Route to appropriate content renderer based on content type
    ContentRenderFunc render_func = nullptr;
    
    switch (content_type_) {
        case ContentType::MAIN_UI:
            render_func = main_ui_render_func_;
            break;
            
        case ContentType::DEBUG:
            render_func = debug_render_func_;
            break;
            
        case ContentType::PLUGIN_UI:
            render_func = plugin_render_func_;
            break;
            
        case ContentType::OCTAVE_UI:
            // Octave UI is rendered separately in RenderOctaveUI()
            // Don't set render_func for it
            break;
            
        case ContentType::MENU:
        case ContentType::SETTINGS:
        case ContentType::NONE:
            // No renderer or placeholder
            break;
    }
    
    // Call the render function if available
    if (render_func && display_) {
        render_func(display_);
    }
}

} // namespace OpenChord