#include "menu_manager.h"
#include "settings_manager.h"
#include "global_settings.h"
#include "../tracks/track_interface.h"
#include "../plugin_interface.h"
#include "../io/io_manager.h"
#include "../../plugins/input/chord_mapping_input.h"  // For ChordMappingInput cast
#include "../../plugins/input/piano_input.h"  // For PianoInput cast
#include "daisy_seed.h"  // For Font_6x8
#include <cstring>
#include <cstdio>
#include <cmath>  // For std::abs

// Forward declaration for plugin casting
namespace OpenChord {
    class IInputPlugin;
    class IEffectPlugin;
}

namespace OpenChord {

// Static menu initialization
Menu MenuManager::main_menu_;
MenuItem MenuManager::main_menu_items_[8];
bool MenuManager::menus_initialized_ = false;

Menu::Menu()
    : title_(nullptr)
    , items_(nullptr)
    , item_count_(0)
{
}

Menu::~Menu() {
}

void Menu::Init(const char* title, MenuItem* items, int item_count) {
    title_ = title;
    items_ = items;
    item_count_ = item_count;
}

MenuManager::MenuManager()
    : display_(nullptr)
    , input_manager_(nullptr)
    , current_track_(nullptr)
    , global_settings_(nullptr)
    , current_menu_type_(MenuType::NONE)
    , current_menu_stack_depth_(0)
    , current_settings_plugin_(nullptr)
    , needs_refresh_(false)
    , temp_menu_count_(0)
{
    // Initialize menu stack
    for (int i = 0; i < MAX_MENU_DEPTH; i++) {
        menu_stack_[i] = nullptr;
        selected_indices_[i] = 0;
    }
}

MenuManager::~MenuManager() {
}

void MenuManager::Init(DisplayManager* display, InputManager* input_manager) {
    display_ = display;
    input_manager_ = input_manager;
    current_track_ = nullptr;
    current_menu_type_ = MenuType::NONE;
    current_menu_stack_depth_ = 0;
    current_settings_plugin_ = nullptr;
    needs_refresh_ = false;
    
    // Initialize static main menu if not already done
    if (!menus_initialized_) {
        // Main menu items will be generated dynamically
        menus_initialized_ = true;
    }
}

void MenuManager::CloseMenu() {
    current_menu_type_ = MenuType::NONE;
    current_menu_stack_depth_ = 0;
    current_settings_plugin_ = nullptr;
    
    // Clear menu stack
    for (int i = 0; i < MAX_MENU_DEPTH; i++) {
        menu_stack_[i] = nullptr;
        selected_indices_[i] = 0;
    }
}

void MenuManager::OpenInputStackMenu() {
    // Close any existing menu and open input stack menu
    // Note: We DON'T preserve current_settings_plugin_ here - let it be restored externally
    // if needed (from settings manager state)
    CloseMenu();
    current_menu_type_ = MenuType::INPUT_STACK;
    GenerateInputStackMenu();
    if (current_menu_stack_depth_ > 0) {
        selected_indices_[0] = 0;
    }
}

void MenuManager::OpenInstrumentMenu() {
    // Close any existing menu and open instrument menu
    CloseMenu();
    current_menu_type_ = MenuType::INSTRUMENT;
    GenerateInstrumentMenu();
    // Instrument menu directly enters settings if available
}

void MenuManager::OpenFXMenu() {
    // Close any existing menu and open FX menu
    CloseMenu();
    current_menu_type_ = MenuType::FX;
    GenerateFXMenu();
    if (current_menu_stack_depth_ > 0) {
        selected_indices_[0] = 0;
    }
}

void MenuManager::OpenMainMenu() {
    // Close any existing menu and open main menu
    CloseMenu();
    current_menu_type_ = MenuType::MAIN;
    GenerateMainMenu();
    if (current_menu_stack_depth_ > 0) {
        selected_indices_[0] = 0;
    }
}

void MenuManager::OpenGlobalSettingsMenu() {
    // Close any existing menu and open global settings menu
    CloseMenu();
    current_menu_type_ = MenuType::GLOBAL_SETTINGS;
    GenerateSystemMenu();  // GenerateSystemMenu creates the global settings menu
    if (current_menu_stack_depth_ > 0) {
        selected_indices_[0] = 0;
    }
}

void MenuManager::NavigateUp() {
    if (!IsOpen()) return;
    
    Menu* menu = GetCurrentMenu();
    if (!menu) return;
    
    // Use current_menu_stack_depth_ - 1 to match GetCurrentSelectedIndex()
    if (current_menu_stack_depth_ <= 0) return;
    int& selected = selected_indices_[current_menu_stack_depth_ - 1];
    selected--;
    if (selected < 0) {
        selected = menu->GetItemCount() - 1;
    }
}

void MenuManager::NavigateDown() {
    if (!IsOpen()) return;
    
    Menu* menu = GetCurrentMenu();
    if (!menu) return;
    
    // Use current_menu_stack_depth_ - 1 to match GetCurrentSelectedIndex()
    if (current_menu_stack_depth_ <= 0) return;
    int& selected = selected_indices_[current_menu_stack_depth_ - 1];
    selected++;
    if (selected >= menu->GetItemCount()) {
        selected = 0;
    }
}

void MenuManager::NavigateEnter() {
    if (!IsOpen()) return;
    
    Menu* menu = GetCurrentMenu();
    if (!menu) return;
    
    int selected = GetCurrentSelectedIndex();
    const MenuItem* item = menu->GetItem(selected);
    if (!item) return;
    
    switch (item->type) {
        case MenuItemType::PLUGIN_SETTINGS:
            // RIGHT enters settings (only if plugin has settings)
            if (item->context) {
                // Context was stored as IPluginWithSettings* in CreatePluginSettingsItem
                // Just cast it back - the pointer offset should be correct since we cast it
                // through ChordMappingInput* when creating the menu item
                current_settings_plugin_ = static_cast<IPluginWithSettings*>(item->context);
            }
            // If context is nullptr, plugin doesn't have settings - do nothing (can't enter settings)
            break;
            
        case MenuItemType::SUBMENU:
            if (item->context) {
                PushMenu(static_cast<Menu*>(item->context));
            }
            break;
            
        case MenuItemType::ACTION:
            if (item->action) {
                item->action(item->context);
            }
            break;
            
        case MenuItemType::SEPARATOR:
            // Do nothing for separators
            break;
    }
}

void MenuManager::ToggleCurrentItem() {
    if (!IsOpen()) return;
    
    Menu* menu = GetCurrentMenu();
    if (!menu) return;
    
    int selected = GetCurrentSelectedIndex();
    const MenuItem* item = menu->GetItem(selected);
    if (!item) return;
    
    // Toggle plugin active state
    if (item->type == MenuItemType::PLUGIN_SETTINGS) {
        // Works for both plugins with settings (item->context != nullptr) and without (item->context == nullptr)
        // We search by name, so context is not required
        if (item->label && current_track_) {
            // The context is stored as IPluginWithSettings* for plugins with settings, but can be nullptr
            // for plugins without settings. We identify the plugin by matching the menu item label 
            // with the plugin name, which works for both cases.
            
            const char* plugin_name = item->label;
            const auto& plugins = current_track_->GetInputPlugins();
            
            // Search for the plugin by name in the track's input plugins
            for (const auto& plugin : plugins) {
                if (plugin && plugin->GetName() && strcmp(plugin->GetName(), plugin_name) == 0) {
                    // Found the plugin! Now we can safely cast it
                    IInputPlugin* input_plugin = plugin.get();
                    if (input_plugin) {
                        // Use Track's SetInputPluginActive to handle exclusive plugin logic
                        bool new_state = !input_plugin->IsActive();
                        current_track_->SetInputPluginActive(input_plugin, new_state);
                        // Force menu refresh to update ON/OFF display
                        needs_refresh_ = true;
                        return;
                    }
                }
            }
            
            // If not found in input plugins, try effects
            const auto& effects = current_track_->GetEffects();
            for (const auto& effect : effects) {
                if (effect && effect->GetName() && strcmp(effect->GetName(), plugin_name) == 0) {
                    IEffectPlugin* effect_plugin = effect.get();
                    if (effect_plugin) {
                        effect_plugin->SetBypass(!effect_plugin->IsBypassed());
                        return;
                    }
                }
            }
            
            // If still not found, the plugin might not exist or the name doesn't match
            // Nothing we can do without RTTI - just return silently
        }
    }
}

void MenuManager::NavigateBack() {
    if (!IsOpen()) return;
    
    // If we're in settings, exit settings first
    if (current_settings_plugin_) {
        current_settings_plugin_ = nullptr;
        // Stay in the menu list (don't close menu, just exit settings view)
        // The menu is still open, we're just exiting settings to show the menu list
        return;
    }
    
    // If we have menu stack levels (submenus), pop one
    if (current_menu_stack_depth_ > 1) {
        // We're in a submenu, pop it to go back to parent
        PopMenu();
        // If we've popped all menus, close
        if (current_menu_stack_depth_ == 0) {
            CloseMenu();
        }
    } else {
        // At top level of current menu type, do NOT close menu
        // Top-level menus can only be closed by pressing the button again
        // This prevents accidental closing with joystick left
        // Do nothing here - just stay in the menu
    }
}

void MenuManager::Update() {
    if (!IsOpen() || !input_manager_) return;
    
    // Handle button input for navigation
    HandleButtonInput();
}

bool MenuManager::UpdateMenuInput(SettingsManager* settings_mgr, IOManager* io_manager, uint32_t current_time_ms) {
    if (!input_manager_ || !io_manager) return false;
    
    bool state_changed = false;
    const float nav_threshold = 0.3f;
    static uint32_t last_nav_time = 0;
    static bool prev_joystick_button_menu = false;
    
    // Check joystick button click for toggle (works in both menu and settings)
    bool joystick_button = false;
    if (io_manager->GetDigital()) {
        joystick_button = io_manager->GetDigital()->WasJoystickButtonPressed();
    }
    
    // Edge detection: trigger on rising edge (false -> true transition)
    bool button_press_edge = joystick_button && !prev_joystick_button_menu;
    
    if (button_press_edge) {
        // Joystick button click: toggle current item (in menu) or toggle setting (in settings)
        bool actually_in_settings = (current_settings_plugin_ != nullptr);
        if (actually_in_settings) {
            // In settings mode: toggle current setting value (for bool/enum)
            if (settings_mgr) {
                settings_mgr->ToggleValue();
                state_changed = true;
            }
        } else {
            // In menu mode: toggle current plugin on/off
            if (IsOpen()) {
                ToggleCurrentItem();
                state_changed = true;
            }
        }
    }
    prev_joystick_button_menu = joystick_button;
    
    // Debounce navigation (every 200ms)
    if (current_time_ms - last_nav_time > 200) {
        IPluginWithSettings* settings_plugin = settings_mgr ? settings_mgr->GetPlugin() : nullptr;
        
        // Get joystick position for navigation
        float joystick_x, joystick_y;
        input_manager_->GetJoystick().GetPosition(&joystick_x, &joystick_y);
        
        if (settings_plugin) {
            // Settings mode
            // Encoder changes values
            float encoder_delta = input_manager_->GetEncoder().GetDelta();
            if (std::abs(encoder_delta) > 0.01f) {
                if (settings_mgr) {
                    settings_mgr->ChangeValue(encoder_delta);
                    state_changed = true;
                }
            }
            
            // Joystick Y: Navigate up/down in settings list
            if (joystick_y > nav_threshold) {
                // UP (joystick down = positive Y): move selection up (decrease index)
                if (settings_mgr) {
                    settings_mgr->MoveSelection(-1);
                    state_changed = true;
                }
                last_nav_time = current_time_ms;
            } else if (joystick_y < -nav_threshold) {
                // DOWN (joystick up = negative Y): move selection down (increase index)
                if (settings_mgr) {
                    settings_mgr->MoveSelection(1);
                    state_changed = true;
                }
                last_nav_time = current_time_ms;
            }
            
            // LEFT: Always go back to menu list from settings
            if (joystick_x < -nav_threshold) {
                NavigateBack();
                // NavigateBack() clears current_settings_plugin_ in menu manager
                // Sync the settings manager to match
                if (settings_mgr) {
                    IPluginWithSettings* menu_plugin = GetCurrentSettingsPlugin();
                    if (menu_plugin != settings_mgr->GetPlugin()) {
                        settings_mgr->SetPlugin(menu_plugin);  // Will be nullptr if we exited settings
                    }
                }
                state_changed = true;
                last_nav_time = current_time_ms;
            }
        } else {
            // Menu mode: handle navigation
            if (joystick_y > nav_threshold) {
                NavigateUp();
                state_changed = true;
                last_nav_time = current_time_ms;
            } else if (joystick_y < -nav_threshold) {
                NavigateDown();
                state_changed = true;
                last_nav_time = current_time_ms;
            } else if (joystick_x > nav_threshold) {
                // RIGHT: Enter settings for selected item
                NavigateEnter();
                state_changed = true;
                last_nav_time = current_time_ms;
                
                // If we entered plugin settings, set it in settings manager
                IPluginWithSettings* plugin = GetCurrentSettingsPlugin();
                if (plugin && settings_mgr) {
                    settings_mgr->SetPlugin(plugin);
                }
            }
        }
    }
    
    return state_changed;
}

void MenuManager::Render() {
    if (!IsHealthy() || !IsOpen()) return;
    
    auto* disp = display_->GetDisplay();
    if (!disp) return;
    
    // If we're in plugin settings mode, don't render menu
    if (current_settings_plugin_) {
        return;  // Settings Manager will handle rendering
    }
    
    Menu* menu = GetCurrentMenu();
    if (!menu) return;
    
    // Render menu title and items
    // Content area starts at y=10
    int y = 10;
    const int line_height = 10;
    const int max_visible_lines = 5;
    
    // Render title
    if (menu->GetTitle()) {
        disp->SetCursor(0, y);
        disp->WriteString(menu->GetTitle(), Font_6x8, true);
        y += line_height;
    }
    
    // Render menu items
    int selected = GetCurrentSelectedIndex();
    int start_index = 0;
    if (selected >= max_visible_lines) {
        start_index = selected - max_visible_lines + 1;
    }
    
    int end_index = start_index + max_visible_lines;
    if (end_index > menu->GetItemCount()) {
        end_index = menu->GetItemCount();
    }
    
    for (int i = start_index; i < end_index; i++) {
        const MenuItem* item = menu->GetItem(i);
        if (!item) continue;
        
        if (item->type == MenuItemType::SEPARATOR) {
            // Draw separator line
            y += 2;
            continue;
        }
        
        const char* prefix = (i == selected) ? "> " : "  ";
        
        // For plugin items, show on/off status
        char status_suffix[8] = "";
        if (item->type == MenuItemType::PLUGIN_SETTINGS && item->label && current_track_) {
            // We can't safely cast from IPluginWithSettings* to IInputPlugin* due to multiple inheritance.
            // Instead, look up the plugin from the track by name (same approach as ToggleCurrentItem).
            // This works for both plugins with settings (item->context != nullptr) and without (item->context == nullptr).
            const char* plugin_name = item->label;
            
            // Search for the plugin in input plugins
            const auto& plugins = current_track_->GetInputPlugins();
            for (const auto& plugin : plugins) {
                if (plugin && plugin->GetName() && strcmp(plugin->GetName(), plugin_name) == 0) {
                    IInputPlugin* input_plugin = plugin.get();
                    if (input_plugin) {
                        snprintf(status_suffix, sizeof(status_suffix), " [%s]", 
                                input_plugin->IsActive() ? "ON" : "OFF");
                        break;
                    }
                }
            }
            
            // If not found in input plugins, try effects
            if (status_suffix[0] == '\0') {
                const auto& effects = current_track_->GetEffects();
                for (const auto& effect : effects) {
                    if (effect && effect->GetName() && strcmp(effect->GetName(), plugin_name) == 0) {
                        IEffectPlugin* effect_plugin = effect.get();
                        if (effect_plugin) {
                            snprintf(status_suffix, sizeof(status_suffix), " [%s]", 
                                    effect_plugin->IsBypassed() ? "OFF" : "ON");
                            break;
                        }
                    }
                }
            }
        }
        
        char buffer[40];
        snprintf(buffer, sizeof(buffer), "%s%s%s", prefix, item->label, status_suffix);
        
        disp->SetCursor(0, y);
        disp->WriteString(buffer, Font_6x8, true);
        y += line_height;
        
        if (y >= 64) break;
    }
}

void MenuManager::HandleJoystickInput(float x, float y) {
    // Joystick navigation is handled in main.cpp with timing-based edge detection
    // This function is kept for potential future use but doesn't handle navigation here
    (void)x;
    (void)y;
}

void MenuManager::HandleButtonInput() {
    // Button input is handled externally via NavigateTo* methods
    // This method can be extended for button repeat/press detection
}

void MenuManager::GenerateMainMenu() {
    // For now, create a simple main menu
    // Later: Generate dynamically based on tracks, plugins, etc.
    
    main_menu_items_[0] = CreateActionItem("Track 1", nullptr, nullptr);
    main_menu_items_[1] = CreateActionItem("System Settings", nullptr, nullptr);
    main_menu_items_[2] = CreateSeparatorItem();
    main_menu_items_[3] = CreateActionItem("Debug View", nullptr, nullptr);
    
    main_menu_.Init("Main Menu", main_menu_items_, 4);
    PushMenu(&main_menu_);
}

void MenuManager::GenerateInputStackMenu() {
    if (!current_track_) return;
    
    // Clear temp items array first
    std::memset(temp_items_, 0, sizeof(temp_items_));
    
    // Generate menu of input plugins
    // First, collect all plugins with their priorities for sorting
    struct PluginEntry {
        IInputPlugin* plugin;
        int priority;
        size_t original_index;
    };
    
    PluginEntry plugin_entries[16];  // Max plugins
    int entry_count = 0;
    const auto& plugins = current_track_->GetInputPlugins();
    
    for (size_t i = 0; i < plugins.size() && entry_count < 16; i++) {
        auto* plugin = plugins[i].get();
        if (!plugin) continue;  // Skip null plugins
        
        const char* name = plugin->GetName();
        if (!name) continue;  // Skip plugins without names
        
        // Include all plugins now (PianoInput is visible)
        plugin_entries[entry_count].plugin = plugin;
        plugin_entries[entry_count].priority = plugin->GetPriority();
        plugin_entries[entry_count].original_index = i;
        entry_count++;
    }
    
    // Sort by priority (lower number = higher priority = appears first)
    for (int i = 0; i < entry_count - 1; i++) {
        for (int j = i + 1; j < entry_count; j++) {
            if (plugin_entries[i].priority > plugin_entries[j].priority) {
                PluginEntry temp = plugin_entries[i];
                plugin_entries[i] = plugin_entries[j];
                plugin_entries[j] = temp;
            }
        }
    }
    
    // Now create menu items in priority order
    int item_count = 0;
    for (int i = 0; i < entry_count && item_count < MAX_TEMP_ITEMS; i++) {
        auto* plugin = plugin_entries[i].plugin;
        const char* name = plugin->GetName();
        
        // Check if plugin supports settings
        // Note: Can't use dynamic_cast due to -fno-rtti
        // With multiple inheritance, we need to handle the pointer offset correctly.
        // 
        // The issue: When a class inherits from both IInputPlugin and IPluginWithSettings,
        // a pointer to IInputPlugin* points to the first base class, while IPluginWithSettings*
        // points to the second base class at a different offset.
        //
        // Solution: Cast through the actual object type. We check by name and cast appropriately.
        
        IPluginWithSettings* settings_plugin = nullptr;
        
        // Check if plugin supports settings by name (since we can't use typeid/dynamic_cast)
        // Plugins that implement IPluginWithSettings need special casting to handle
        // multiple inheritance pointer offset correctly
        if (name) {
            if (strcmp(name, "Chords") == 0) {
                // This is ChordMappingInput - cast to the actual object type first
                ChordMappingInput* chord_plugin = static_cast<ChordMappingInput*>(plugin);
                settings_plugin = static_cast<IPluginWithSettings*>(chord_plugin);
            } else if (strcmp(name, "Notes") == 0) {
                // This is PianoInput - cast to the actual object type first
                PianoInput* piano_plugin = static_cast<PianoInput*>(plugin);
                settings_plugin = static_cast<IPluginWithSettings*>(piano_plugin);
            }
        }
        
        // Add all plugins to menu (not just ones with settings)
        // Plugins without settings can still be toggled on/off
        // For plugins with settings, we can navigate into settings with RIGHT
        if (settings_plugin) {
            // Plugin has settings - create settings item
            temp_items_[item_count++] = CreatePluginSettingsItem(
                plugin->GetName(), settings_plugin);
        } else if (name) {
            // Plugin doesn't have settings - still create a menu item for toggling
            // Use PLUGIN_SETTINGS type with nullptr - ToggleCurrentItem will find it by name
            // We need to ensure the label pointer is valid (GetName() returns const char* to string literal)
            MenuItem item;
            item.label = name;  // String literal pointer, safe to store
            item.type = MenuItemType::PLUGIN_SETTINGS;
            item.context = nullptr;  // No settings plugin, but ToggleCurrentItem will find it by name
            item.action = nullptr;
            item.shortcut_hint = nullptr;
            temp_items_[item_count++] = item;
        }
    }
    
    if (item_count > 0) {
        // No title - system bar already shows "Input"
        temp_menus_[0].Init(nullptr, temp_items_, item_count);
        PushMenu(&temp_menus_[0]);
    }
}

void MenuManager::GenerateInstrumentMenu() {
    // Show list of instruments (similar to input stack)
    // For now, there's typically only one instrument, but we show it as a list for consistency
    if (!current_track_) return;
    
    auto* instrument = current_track_->GetInstrument();
    if (!instrument) return;
    
    // Check if instrument supports settings
    IPluginWithSettings* settings_plugin = reinterpret_cast<IPluginWithSettings*>(instrument);
    if (settings_plugin) {
        temp_items_[0] = CreatePluginSettingsItem(instrument->GetName(), settings_plugin);
        temp_menus_[2].Init("Instruments", temp_items_, 1);
        PushMenu(&temp_menus_[2]);
    }
}

void MenuManager::GenerateFXMenu() {
    // Similar to input stack
    if (!current_track_) return;
    
    int item_count = 0;
    const auto& effects = current_track_->GetEffects();
    
    for (size_t i = 0; i < effects.size() && item_count < MAX_TEMP_ITEMS; i++) {
        auto* effect = effects[i].get();
        
        // Check if effect supports settings
        // Note: Can't use dynamic_cast due to -fno-rtti
        // We'll try to use it as IPluginWithSettings and SettingsManager will validate
        IPluginWithSettings* settings_plugin = reinterpret_cast<IPluginWithSettings*>(effect);
        temp_items_[item_count++] = CreatePluginSettingsItem(
            effect->GetName(), settings_plugin);
    }
    
    if (item_count > 0) {
        temp_menus_[1].Init("Effects", temp_items_, item_count);
        PushMenu(&temp_menus_[1]);
    }
}

void MenuManager::GenerateTrackMenu(int track_index) {
    // TODO: Implement track-specific menu
    (void)track_index;
}

void MenuManager::GenerateSystemMenu() {
    // Generate global settings menu
    // Directly enter settings view (similar to instrument menu when it has settings)
    
    if (!global_settings_) {
        return;  // No global settings available
    }
    
    // Directly set the global settings plugin (similar to instrument menu)
    // This bypasses the menu list and goes straight to settings view
    current_settings_plugin_ = global_settings_;
}

MenuItem MenuManager::CreateSubmenuItem(const char* label, Menu* submenu) {
    MenuItem item;
    item.label = label;
    item.type = MenuItemType::SUBMENU;
    item.context = submenu;
    item.action = nullptr;
    item.shortcut_hint = nullptr;
    return item;
}

MenuItem MenuManager::CreatePluginSettingsItem(const char* label, IPluginWithSettings* plugin) {
    MenuItem item;
    item.label = label;
    item.type = MenuItemType::PLUGIN_SETTINGS;
    item.context = plugin;
    item.action = nullptr;
    item.shortcut_hint = nullptr;
    return item;
}

MenuItem MenuManager::CreateActionItem(const char* label, MenuActionCallback action, void* context) {
    MenuItem item;
    item.label = label;
    item.type = MenuItemType::ACTION;
    item.context = context;
    item.action = action;
    item.shortcut_hint = nullptr;
    return item;
}

MenuItem MenuManager::CreateSeparatorItem() {
    MenuItem item;
    item.label = "---";
    item.type = MenuItemType::SEPARATOR;
    item.context = nullptr;
    item.action = nullptr;
    item.shortcut_hint = nullptr;
    return item;
}

void MenuManager::PushMenu(Menu* menu) {
    if (current_menu_stack_depth_ >= MAX_MENU_DEPTH - 1) {
        return;  // Stack overflow
    }
    
    menu_stack_[current_menu_stack_depth_] = menu;
    selected_indices_[current_menu_stack_depth_] = 0;
    current_menu_stack_depth_++;
}

void MenuManager::PopMenu() {
    if (current_menu_stack_depth_ <= 0) {
        return;
    }
    
    current_menu_stack_depth_--;
    menu_stack_[current_menu_stack_depth_] = nullptr;
}

Menu* MenuManager::GetCurrentMenu() const {
    if (current_menu_stack_depth_ <= 0) {
        return nullptr;
    }
    return menu_stack_[current_menu_stack_depth_ - 1];
}

int MenuManager::GetCurrentSelectedIndex() const {
    if (current_menu_stack_depth_ <= 0) {
        return 0;
    }
    return selected_indices_[current_menu_stack_depth_ - 1];
}

const char* MenuManager::GetContextName() const {
    if (current_settings_plugin_) {
        return "Settings";
    }
    
    switch (current_menu_type_) {
        case MenuType::INPUT_STACK:
            return "Input";
        case MenuType::INSTRUMENT:
            return "Instrument";
        case MenuType::FX:
            return "FX";
        case MenuType::MAIN:
            return "Menu";
        case MenuType::GLOBAL_SETTINGS:
            return "Global";
        default:
            return "";
    }
}

} // namespace OpenChord
