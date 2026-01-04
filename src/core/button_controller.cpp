#include "button_controller.h"
#include "io/input_manager.h"
#include "ui/menu_manager.h"
#include "ui/settings_manager.h"
#include "ui/plugin_settings.h"
#include "transport_control.h"
#include "io/io_manager.h"
#ifdef DEBUG_SCREEN_ENABLED
#include "ui/debug_screen.h"
#endif
#include <cstring>

namespace OpenChord {

ButtonController::ButtonController()
    : hw_(nullptr)
    , input_manager_(nullptr)
    , menu_mgr_(nullptr)
    , settings_mgr_(nullptr)
    , transport_control_(nullptr)
    , ui_manager_(nullptr)
    , io_manager_(nullptr)
#ifdef DEBUG_SCREEN_ENABLED
    , debug_screen_(nullptr)
#endif
{
    // Initialize button states
    memset(&input_button_, 0, sizeof(input_button_));
    memset(&record_button_, 0, sizeof(record_button_));
    memset(&instrument_button_, 0, sizeof(instrument_button_));
    memset(&fx_button_, 0, sizeof(fx_button_));
}

ButtonController::~ButtonController() {
}

void ButtonController::Init(daisy::DaisySeed* hw, InputManager* input_manager,
                            MenuManager* menu_mgr, SettingsManager* settings_mgr,
                            TransportControl* transport_control, UIManager* ui_manager,
                            IOManager* io_manager) {
    hw_ = hw;
    input_manager_ = input_manager;
    menu_mgr_ = menu_mgr;
    settings_mgr_ = settings_mgr;
    transport_control_ = transport_control;
    ui_manager_ = ui_manager;
    io_manager_ = io_manager;
    
    // Reset button states
    input_button_.prev_pressed = false;
    input_button_.hold_start_time = 0;
    input_button_.menu_open = false;
    
    record_button_.prev_pressed = false;
    record_button_.hold_start_time = 0;
    record_button_.menu_open = false;
    
    instrument_button_.prev_pressed = false;
    instrument_button_.hold_start_time = 0;
    instrument_button_.menu_open = false;
    
    fx_button_.prev_pressed = false;
    fx_button_.hold_start_time = 0;
    fx_button_.menu_open = false;
}

void ButtonController::Update() {
    if (!hw_ || !input_manager_) return;
    
    // Check if debug mode is active
    if (IsDebugModeActive()) {
        // Debug mode - reset all hold timers and close menus
        if (input_button_.menu_open && menu_mgr_) menu_mgr_->CloseMenu();
        if (record_button_.menu_open && menu_mgr_) menu_mgr_->CloseMenu();
        if (instrument_button_.menu_open && menu_mgr_) menu_mgr_->CloseMenu();
        if (fx_button_.menu_open && menu_mgr_) menu_mgr_->CloseMenu();
        
        input_button_.hold_start_time = 0;
        record_button_.hold_start_time = 0;
        instrument_button_.hold_start_time = 0;
        fx_button_.hold_start_time = 0;
        
        input_button_.menu_open = false;
        record_button_.menu_open = false;
        instrument_button_.menu_open = false;
        fx_button_.menu_open = false;
        return;
    }
    
    // Update each button
    UpdateInputButton();
    UpdateRecordButton();
    UpdateInstrumentButton();
    UpdateFXButton();
}

void ButtonController::UpdateInputButton() {
    if (!hw_ || !input_manager_ || !menu_mgr_) return;
    
    bool input_pressed = input_manager_->GetButtons().IsSystemButtonPressed(SystemButton::INPUT);
    
    if (input_pressed) {
        if (input_button_.hold_start_time == 0) {
            input_button_.hold_start_time = hw_->system.GetNow();
            
            // If menu is in toggle mode, this is a press to close it
            if (menu_mgr_->IsOpen() && menu_mgr_->GetCurrentMenuType() == MenuManager::MenuType::INPUT_STACK &&
                menu_mgr_->IsInToggleMode()) {
                // Menu is in toggle mode, user pressed button to close - handle on release
            }
        } else {
            uint32_t hold_duration = hw_->system.GetNow() - input_button_.hold_start_time;
            
            // Check if menu is in toggle mode (button pressed to close)
            if (menu_mgr_->IsOpen() && menu_mgr_->GetCurrentMenuType() == MenuManager::MenuType::INPUT_STACK &&
                menu_mgr_->IsInToggleMode()) {
                // Menu is in toggle mode, user is holding to close - handle on release
            } else if (hold_duration >= BUTTON_HOLD_THRESHOLD_MS && !input_button_.menu_open) {
                // INPUT held - open Input Stack menu
                CloseAllMenusExcept(MenuManager::MenuType::INPUT_STACK);
                menu_mgr_->OpenInputStackMenu();
                menu_mgr_->SetMenuOpenTime(hw_->system.GetNow());
                input_button_.menu_open = true;
            }
        }
    } else {
        // INPUT button released
        if (input_button_.hold_start_time > 0) {
            uint32_t press_duration = hw_->system.GetNow() - input_button_.hold_start_time;
            uint32_t current_time = hw_->system.GetNow();
            
            bool menu_was_open = (input_button_.menu_open && menu_mgr_->IsOpen() &&
                                 menu_mgr_->GetCurrentMenuType() == MenuManager::MenuType::INPUT_STACK);
            bool was_in_toggle_mode = (menu_was_open && menu_mgr_->IsInToggleMode());
            
            if (was_in_toggle_mode) {
                // Menu was in toggle mode, user pressed button to close
                menu_mgr_->CloseMenu();
                input_button_.menu_open = false;
            } else if (menu_was_open) {
                // Menu was just opened, check if we should toggle
                uint32_t menu_open_time = menu_mgr_->GetMenuOpenTime();
                uint32_t time_since_open = current_time - menu_open_time;
                
                if (time_since_open < MENU_TOGGLE_WINDOW_MS) {
                    // Released within 1 second - enter toggle mode (menu stays open)
                    menu_mgr_->SetToggleMode(true);
                } else {
                    // Held past 1 second - close menu
                    menu_mgr_->CloseMenu();
                    input_button_.menu_open = false;
                }
            } else {
                // Menu wasn't open - normal tap/hold behavior
                bool was_tap = (press_duration < BUTTON_HOLD_THRESHOLD_MS);
                
                // Trigger play/pause only if it was a tap (not a hold)
                if (was_tap && transport_control_) {
                    transport_control_->HandleCombo(0);  // 0 = Play/Pause
                }
            }
            
            input_button_.hold_start_time = 0;
            if (!was_in_toggle_mode && !menu_was_open) {
                input_button_.menu_open = false;
            }
        }
    }
    
    input_button_.prev_pressed = input_pressed;
}

void ButtonController::UpdateRecordButton() {
    if (!hw_ || !input_manager_ || !menu_mgr_) return;
    
    bool record_pressed = input_manager_->GetButtons().IsSystemButtonPressed(SystemButton::RECORD);
    
    if (record_pressed) {
        if (record_button_.hold_start_time == 0) {
            record_button_.hold_start_time = hw_->system.GetNow();
            
            // If menu is in toggle mode, this is a press to close it
            if (menu_mgr_->IsOpen() && menu_mgr_->GetCurrentMenuType() == MenuManager::MenuType::GLOBAL_SETTINGS &&
                menu_mgr_->IsInToggleMode()) {
                // Menu is in toggle mode, user pressed button to close - handle on release
            }
        } else {
            uint32_t hold_duration = hw_->system.GetNow() - record_button_.hold_start_time;
            
            // Check if menu is in toggle mode (button pressed to close)
            if (menu_mgr_->IsOpen() && menu_mgr_->GetCurrentMenuType() == MenuManager::MenuType::GLOBAL_SETTINGS &&
                menu_mgr_->IsInToggleMode()) {
                // Menu is in toggle mode, user is holding to close - handle on release
            } else if (hold_duration >= BUTTON_HOLD_THRESHOLD_MS && !record_button_.menu_open) {
                // RECORD held - open Global Settings menu
                CloseAllMenusExcept(MenuManager::MenuType::GLOBAL_SETTINGS);
                menu_mgr_->OpenGlobalSettingsMenu();
                menu_mgr_->SetMenuOpenTime(hw_->system.GetNow());
                if (settings_mgr_) {
                    IPluginWithSettings* plugin = menu_mgr_->GetCurrentSettingsPlugin();
                    if (plugin) {
                        settings_mgr_->SetPlugin(plugin);
                    }
                }
                record_button_.menu_open = true;
            }
        }
    } else {
        // RECORD button released
        if (record_button_.hold_start_time > 0) {
            uint32_t press_duration = hw_->system.GetNow() - record_button_.hold_start_time;
            uint32_t current_time = hw_->system.GetNow();
            
            bool menu_was_open = (record_button_.menu_open && menu_mgr_->IsOpen() &&
                                 menu_mgr_->GetCurrentMenuType() == MenuManager::MenuType::GLOBAL_SETTINGS);
            bool was_in_toggle_mode = (menu_was_open && menu_mgr_->IsInToggleMode());
            
            if (was_in_toggle_mode) {
                // Menu was in toggle mode, user pressed button to close
                menu_mgr_->CloseMenu();
                if (settings_mgr_) {
                    settings_mgr_->SetPlugin(nullptr);
                }
                record_button_.menu_open = false;
            } else if (menu_was_open) {
                // Menu was just opened, check if we should toggle
                uint32_t menu_open_time = menu_mgr_->GetMenuOpenTime();
                uint32_t time_since_open = current_time - menu_open_time;
                
                if (time_since_open < MENU_TOGGLE_WINDOW_MS) {
                    // Released within 1 second - enter toggle mode (menu stays open)
                    menu_mgr_->SetToggleMode(true);
                } else {
                    // Held past 1 second - close menu
                    menu_mgr_->CloseMenu();
                    if (settings_mgr_) {
                        settings_mgr_->SetPlugin(nullptr);
                    }
                    record_button_.menu_open = false;
                }
            } else {
                // Menu wasn't open - normal tap/hold behavior
                bool was_tap = (press_duration < BUTTON_HOLD_THRESHOLD_MS);
                
                // Trigger record only if it was a tap (not a hold)
                if (was_tap && transport_control_) {
                    transport_control_->HandleCombo(1);  // 1 = Record toggle
                }
            }
            
            record_button_.hold_start_time = 0;
            if (!was_in_toggle_mode && !menu_was_open) {
                record_button_.menu_open = false;
            }
        }
    }
    
    record_button_.prev_pressed = record_pressed;
}

void ButtonController::UpdateInstrumentButton() {
    if (!hw_ || !input_manager_ || !menu_mgr_) return;
    
    bool instrument_pressed = input_manager_->GetButtons().IsSystemButtonPressed(SystemButton::INSTRUMENT);
    
    if (instrument_pressed) {
        if (instrument_button_.hold_start_time == 0) {
            instrument_button_.hold_start_time = hw_->system.GetNow();
            
            // If menu is in toggle mode, this is a press to close it
            if (menu_mgr_->IsOpen() && menu_mgr_->GetCurrentMenuType() == MenuManager::MenuType::INSTRUMENT &&
                menu_mgr_->IsInToggleMode()) {
                // Menu is in toggle mode, user pressed button to close - handle on release
            }
        } else {
            uint32_t hold_duration = hw_->system.GetNow() - instrument_button_.hold_start_time;
            
            // Check if menu is in toggle mode (button pressed to close)
            if (menu_mgr_->IsOpen() && menu_mgr_->GetCurrentMenuType() == MenuManager::MenuType::INSTRUMENT &&
                menu_mgr_->IsInToggleMode()) {
                // Menu is in toggle mode, user is holding to close - handle on release
            } else if (hold_duration >= BUTTON_HOLD_THRESHOLD_MS && !instrument_button_.menu_open) {
                // INSTRUMENT held - open Instrument menu
                CloseAllMenusExcept(MenuManager::MenuType::INSTRUMENT);
                menu_mgr_->OpenInstrumentMenu();
                menu_mgr_->SetMenuOpenTime(hw_->system.GetNow());
                instrument_button_.menu_open = true;
            }
        }
    } else {
        // INSTRUMENT button released
        if (instrument_button_.hold_start_time > 0) {
            uint32_t press_duration = hw_->system.GetNow() - instrument_button_.hold_start_time;
            uint32_t current_time = hw_->system.GetNow();
            
            bool menu_was_open = (instrument_button_.menu_open && menu_mgr_->IsOpen() &&
                                 menu_mgr_->GetCurrentMenuType() == MenuManager::MenuType::INSTRUMENT);
            bool was_in_toggle_mode = (menu_was_open && menu_mgr_->IsInToggleMode());
            
            if (was_in_toggle_mode) {
                // Menu was in toggle mode, user pressed button to close
                menu_mgr_->CloseMenu();
                instrument_button_.menu_open = false;
            } else if (menu_was_open) {
                // Menu was just opened, check if we should toggle
                uint32_t menu_open_time = menu_mgr_->GetMenuOpenTime();
                uint32_t time_since_open = current_time - menu_open_time;
                
                if (time_since_open < MENU_TOGGLE_WINDOW_MS) {
                    // Released within 1 second - enter toggle mode (menu stays open)
                    menu_mgr_->SetToggleMode(true);
                } else {
                    // Held past 1 second - close menu
                    menu_mgr_->CloseMenu();
                    instrument_button_.menu_open = false;
                }
            }
            
            instrument_button_.hold_start_time = 0;
            if (!was_in_toggle_mode && !menu_was_open) {
                instrument_button_.menu_open = false;
            }
        }
    }
    
    instrument_button_.prev_pressed = instrument_pressed;
}

void ButtonController::UpdateFXButton() {
    if (!hw_ || !input_manager_ || !menu_mgr_) return;
    
    bool fx_pressed = input_manager_->GetButtons().IsSystemButtonPressed(SystemButton::FX);
    
    if (fx_pressed) {
        if (fx_button_.hold_start_time == 0) {
            fx_button_.hold_start_time = hw_->system.GetNow();
            
            // If menu is in toggle mode, this is a press to close it
            if (menu_mgr_->IsOpen() && menu_mgr_->GetCurrentMenuType() == MenuManager::MenuType::FX &&
                menu_mgr_->IsInToggleMode()) {
                // Menu is in toggle mode, user pressed button to close - handle on release
            }
        } else {
            uint32_t hold_duration = hw_->system.GetNow() - fx_button_.hold_start_time;
            
            // Check if menu is in toggle mode (button pressed to close)
            if (menu_mgr_->IsOpen() && menu_mgr_->GetCurrentMenuType() == MenuManager::MenuType::FX &&
                menu_mgr_->IsInToggleMode()) {
                // Menu is in toggle mode, user is holding to close - handle on release
            } else if (hold_duration >= BUTTON_HOLD_THRESHOLD_MS && !fx_button_.menu_open) {
                // FX held - open FX menu
                CloseAllMenusExcept(MenuManager::MenuType::FX);
                menu_mgr_->OpenFXMenu();
                menu_mgr_->SetMenuOpenTime(hw_->system.GetNow());
                fx_button_.menu_open = true;
            }
        }
    } else {
        // FX button released
        if (fx_button_.hold_start_time > 0) {
            uint32_t press_duration = hw_->system.GetNow() - fx_button_.hold_start_time;
            uint32_t current_time = hw_->system.GetNow();
            
            bool menu_was_open = (fx_button_.menu_open && menu_mgr_->IsOpen() &&
                                 menu_mgr_->GetCurrentMenuType() == MenuManager::MenuType::FX);
            bool was_in_toggle_mode = (menu_was_open && menu_mgr_->IsInToggleMode());
            
            if (was_in_toggle_mode) {
                // Menu was in toggle mode, user pressed button to close
                menu_mgr_->CloseMenu();
                fx_button_.menu_open = false;
            } else if (menu_was_open) {
                // Menu was just opened, check if we should toggle
                uint32_t menu_open_time = menu_mgr_->GetMenuOpenTime();
                uint32_t time_since_open = current_time - menu_open_time;
                
                if (time_since_open < MENU_TOGGLE_WINDOW_MS) {
                    // Released within 1 second - enter toggle mode (menu stays open)
                    menu_mgr_->SetToggleMode(true);
                } else {
                    // Held past 1 second - close menu
                    menu_mgr_->CloseMenu();
                    fx_button_.menu_open = false;
                }
            }
            
            fx_button_.hold_start_time = 0;
            if (!was_in_toggle_mode && !menu_was_open) {
                fx_button_.menu_open = false;
            }
        }
    }
    
    fx_button_.prev_pressed = fx_pressed;
}

void ButtonController::CloseAllMenusExcept(MenuManager::MenuType except_type) {
    if (!menu_mgr_) return;
    
    // Close any other open menus by resetting their flags
    input_button_.menu_open = false;
    record_button_.menu_open = false;
    instrument_button_.menu_open = false;
    fx_button_.menu_open = false;
    
    // OpenInputStackMenu/OpenInstrumentMenu/etc will close any existing menu automatically
}

bool ButtonController::IsDebugModeActive() const {
#ifdef DEBUG_SCREEN_ENABLED
    return debug_screen_ && debug_screen_->IsEnabled();
#else
    return false;
#endif
}

} // namespace OpenChord

