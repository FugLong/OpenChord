#include "ui_manager.h"
#include "system_bar.h"
#include "content_area.h"
#include <new>

namespace OpenChord {

UIManager::UIManager()
    : display_(nullptr)
    , input_manager_(nullptr)
    , io_manager_(nullptr)
    , system_bar_(nullptr)
    , content_area_(nullptr)
    , content_type_(ContentType::NONE)
    , main_ui_render_func_(nullptr)
    , debug_render_func_(nullptr)
    , plugin_render_func_(nullptr)
    , current_track_(nullptr)
    , render_interval_ms_(100)  // 10 FPS
    , last_render_time_(0)
    , needs_refresh_(true)
{
    // Allocate UI components
    system_bar_ = new (std::nothrow) SystemBar();
    content_area_ = new (std::nothrow) ContentArea();
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
    
    // Render periodically
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
    
    // Render content area first (so system bar can render on top)
    RenderContent();
    
    // Render system bar on top (always visible)
    if (system_bar_) {
        system_bar_->Render();
    }
    
    // Update display (once at the end)
    disp->Update();
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