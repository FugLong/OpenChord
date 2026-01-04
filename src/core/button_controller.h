#pragma once

#include "io/input_manager.h"
#include "ui/menu_manager.h"
#include "transport_control.h"
#include "daisy_seed.h"
#include <cstdint>

// IOManager is at global scope
class IOManager;

namespace OpenChord {

// Forward declarations
class UIManager;
class SettingsManager;

#ifdef DEBUG_SCREEN_ENABLED
class DebugScreen;
#endif

/**
 * Button Controller
 * 
 * Handles button hold detection, menu opening/closing, and transport controls.
 * Consolidates the duplicated button handling logic from main.cpp.
 */
class ButtonController {
public:
    ButtonController();
    ~ButtonController();
    
    // Initialization
    void Init(daisy::DaisySeed* hw, InputManager* input_manager, 
              MenuManager* menu_mgr, SettingsManager* settings_mgr,
              TransportControl* transport_control, UIManager* ui_manager,
              IOManager* io_manager);
    
#ifdef DEBUG_SCREEN_ENABLED
    void SetDebugScreen(DebugScreen* debug_screen) { debug_screen_ = debug_screen; }
#endif
    
    /**
     * Update button handling
     * Call this from the main loop
     */
    void Update();
    
private:
    // Button state tracking
    struct ButtonState {
        bool prev_pressed;
        uint32_t hold_start_time;
        bool menu_open;
    };
    
    daisy::DaisySeed* hw_;
    InputManager* input_manager_;
    MenuManager* menu_mgr_;
    SettingsManager* settings_mgr_;
    TransportControl* transport_control_;
    UIManager* ui_manager_;
    IOManager* io_manager_;
    
#ifdef DEBUG_SCREEN_ENABLED
    DebugScreen* debug_screen_;
#endif
    
    // Button states
    ButtonState input_button_;
    ButtonState record_button_;
    ButtonState instrument_button_;
    ButtonState fx_button_;
    
    // Constants
    static constexpr uint32_t BUTTON_HOLD_THRESHOLD_MS = 250;
    static constexpr uint32_t MENU_TOGGLE_WINDOW_MS = 1000;
    
    // Button handling methods
    void UpdateInputButton();
    void UpdateRecordButton();
    void UpdateInstrumentButton();
    void UpdateFXButton();
    
    // Helper methods
    void HandleButtonPress(ButtonState& state, MenuManager::MenuType menu_type, 
                          bool is_toggle_menu_button);
    void HandleButtonRelease(ButtonState& state, MenuManager::MenuType menu_type,
                            int transport_combo, bool has_settings_plugin);
    void CloseAllMenusExcept(MenuManager::MenuType except_type);
    bool IsDebugModeActive() const;
};

} // namespace OpenChord

