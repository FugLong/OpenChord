#pragma once

#include "../io/display_manager.h"
#include "../io/input_manager.h"
#include "plugin_settings.h"
#include <cstdint>

namespace OpenChord {

// Forward declarations
class Track;
class IPluginWithSettings;
class SettingsManager;
class GlobalSettings;

/**
 * Menu Item Types
 */
enum class MenuItemType {
    SUBMENU,        // Opens a submenu
    PLUGIN_SETTINGS,// Opens settings for a plugin
    ACTION,         // Executes an action (callback)
    SEPARATOR       // Visual separator (non-interactive)
};

/**
 * Menu Action Callback
 * Return true if action was handled, false otherwise
 */
typedef bool (*MenuActionCallback)(void* context);

/**
 * Menu Item Definition
 */
struct MenuItem {
    const char* label;              // Display text
    MenuItemType type;              // Type of menu item
    void* context;                  // Context data (submenu pointer, plugin pointer, etc.)
    MenuActionCallback action;      // For ACTION type items
    const char* shortcut_hint;      // Optional hint (e.g., "INPUT" button)
};

/**
 * Menu Definition
 * Represents a menu level with a list of items
 */
class Menu {
public:
    Menu();
    ~Menu();
    
    void Init(const char* title, MenuItem* items, int item_count);
    
    const char* GetTitle() const { return title_; }
    const MenuItem* GetItems() const { return items_; }
    int GetItemCount() const { return item_count_; }
    
    const MenuItem* GetItem(int index) const {
        if (index < 0 || index >= item_count_) return nullptr;
        return &items_[index];
    }
    
private:
    const char* title_;
    MenuItem* items_;
    int item_count_;
};

/**
 * Menu Manager - Handles hierarchical menu navigation and rendering
 * 
 * Responsibilities:
 * - Menu hierarchy navigation (breadcrumbs)
 * - Joystick/button input handling
 * - Menu rendering
 * - Navigation to settings from menu
 */
class MenuManager {
public:
    enum class NavigationButton {
        INPUT,      // INPUT button
        INSTRUMENT, // INSTRUMENT button
        FX          // FX button
    };
    
    MenuManager();
    ~MenuManager();
    
    // Initialization
    void Init(DisplayManager* display, InputManager* input_manager);
    
    // Menu state
    bool IsOpen() const { return current_menu_type_ != MenuType::NONE; }
    void CloseMenu();  // Close menu (returns to normal mode)
    
    // Navigation
    void NavigateUp();      // Joystick UP
    void NavigateDown();    // Joystick DOWN
    void NavigateEnter();   // Joystick RIGHT - enter settings
    void NavigateBack();    // Joystick LEFT - go back (or exit settings)
    void ToggleCurrentItem();  // Toggle current plugin on/off (called when LEFT pressed in list)
    
    // Button-based menu opening (each button opens only its own menu)
    void OpenInputStackMenu();     // Press INPUT button - opens input plugins menu
    void OpenInstrumentMenu();     // Press INSTRUMENT button - opens instrument settings
    void OpenFXMenu();             // Press FX button - opens effects menu
    void OpenMainMenu();           // Opens main system menu (separate access method)
    void OpenGlobalSettingsMenu(); // Opens global device settings menu (e.g., via RECORD hold)
    
    // Get current menu type
    enum class MenuType {
        NONE,           // No menu open
        INPUT_STACK,    // Input stack menu
        INSTRUMENT,     // Instrument menu
        FX,             // Effects menu
        MAIN,           // Main system menu
        GLOBAL_SETTINGS // Global device settings menu
    };
    MenuType GetCurrentMenuType() const { return current_menu_type_; }
    
    // Update (call from main loop to handle input)
    void Update();
    
    // Handle all menu-related input processing (button presses, joystick navigation, settings)
    // This centralizes all menu input handling that was previously in main.cpp
    // Returns true if menu state changed (needs UI refresh)
    bool UpdateMenuInput(SettingsManager* settings_mgr, IOManager* io_manager, uint32_t current_time_ms);
    
    // Rendering
    void Render();
    
    // Get current plugin for settings (if in plugin settings)
    IPluginWithSettings* GetCurrentSettingsPlugin() const { return current_settings_plugin_; }
    
    // Set current plugin for settings (used to restore menu memory)
    void SetCurrentSettingsPlugin(IPluginWithSettings* plugin) { current_settings_plugin_ = plugin; }
    
    // Set track context (for menu generation)
    void SetTrack(Track* track) { current_track_ = track; }
    
    // Set global settings (for global settings menu)
    void SetGlobalSettings(GlobalSettings* global_settings) { global_settings_ = global_settings; }
    
    // Request immediate refresh (for when plugin state changes)
    void RequestRefresh() { needs_refresh_ = true; }
    
    // Get context name for system bar
    const char* GetContextName() const;
    
    // Health check
    bool IsHealthy() const { return display_ != nullptr && display_->IsHealthy(); }
    
private:
    DisplayManager* display_;
    InputManager* input_manager_;
    Track* current_track_;
    GlobalSettings* global_settings_;
    
    MenuType current_menu_type_;  // What type of menu is currently open
    int current_menu_stack_depth_;
    static constexpr int MAX_MENU_DEPTH = 8;
    Menu* menu_stack_[MAX_MENU_DEPTH];
    int selected_indices_[MAX_MENU_DEPTH];  // Selected index at each level
    
    IPluginWithSettings* current_settings_plugin_;
    
    bool needs_refresh_;  // Flag to request immediate UI refresh
    
    // Menu generation
    void GenerateMainMenu();
    void GenerateTrackMenu(int track_index);
    void GenerateInputStackMenu();
    void GenerateInstrumentMenu();
    void GenerateFXMenu();
    void GenerateSystemMenu();
    
    // Menu item creation helpers
    MenuItem CreateSubmenuItem(const char* label, Menu* submenu);
    MenuItem CreatePluginSettingsItem(const char* label, IPluginWithSettings* plugin);
    MenuItem CreateActionItem(const char* label, MenuActionCallback action, void* context);
    MenuItem CreateSeparatorItem();
    
    // Navigation helpers
    void PushMenu(Menu* menu);
    void PopMenu();
    Menu* GetCurrentMenu() const;
    int GetCurrentSelectedIndex() const;
    
    // Input handling
    void HandleJoystickInput(float x, float y);
    void HandleButtonInput();
    
    // Temporary menu storage (for dynamically generated menus)
    static constexpr int MAX_TEMP_ITEMS = 16;
    MenuItem temp_items_[MAX_TEMP_ITEMS];
    Menu temp_menus_[4];  // Temporary menu storage
    int temp_menu_count_;
    
    // Static menu items for main menu
    static Menu main_menu_;
    static MenuItem main_menu_items_[8];
    static bool menus_initialized_;
};

} // namespace OpenChord
