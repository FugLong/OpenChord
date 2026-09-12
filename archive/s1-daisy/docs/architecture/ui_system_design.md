# UI System Design

[← Back to Documentation](../README.md)

## Overview

The OpenChord UI system provides a consistent, modular interface for navigating settings, viewing information, and controlling the device. It follows a layered approach with a permanent system bar and plugin-driven content area.

## Core Principles

1. **Always-visible system information** - Battery, track, active plugin/context shown in persistent bars
2. **Plugin-driven content** - Plugins can render their own UI in the content area
3. **Consistent navigation** - Same controls work across all contexts
4. **Modal settings access** - Quick access to plugin settings without leaving performance mode
5. **Clean separation** - System UI (bars) vs Content UI (plugin-driven)

## UI Layout

```
┌─────────────────────────────────────┐
│ SYSTEM BAR (permanent, always on)   │  ← Battery %, Track name, Current context
├─────────────────────────────────────┤
│                                     │
│                                     │
│    CONTENT AREA                     │  ← Plugin-driven or system UI
│    (128x48 pixels)                  │  ← Can be controlled by any plugin
│                                     │
│                                     │
├─────────────────────────────────────┤
│ STATUS BAR (optional, contextual)   │  ← Additional info, hints, shortcuts
└─────────────────────────────────────┘
```

**Display specs:**
- Total: 128x64 pixels (OLED SSD1306)
- System Bar (top): 128x8 pixels (1 line)
- Content Area: 128x48 pixels
- Status Bar (bottom): 128x8 pixels (optional, contextual)

## System Bars

### Top System Bar (Always Visible)

Displays:
- **Left**: Battery percentage (e.g., "85%") or charging indicator
- **Center**: Current track name/number (e.g., "Track 1" or "Chord Mode")
- **Right**: Current context/mode indicator
  - Normal: "-" or empty
  - Settings mode: "SETTINGS" or plugin name
  - Menu mode: "MENU"
  - Debug: "DEBUG"

### Bottom Status Bar (Contextual)

Only shows when relevant:
- Navigation hints: "↑↓ nav  ◄► change"
- Active plugin name: "Chord Mapping"
- Parameter value: "Key: C Major"
- Empty in normal operation mode

## UI States / Modes

### 1. Normal Performance Mode (Default)

**Behavior:**
- System bar shows track and battery
- Content area shows plugin's default view (e.g., chord name for chord mapping)
- Bottom bar is empty or minimal

**Navigation:**
- No special navigation active
- Buttons control music/performance
- Joystick controls performance features

### 2. Menu Mode

**Activation:**
- **Tap INPUT button** - Opens/closes main menu
- **Press INPUT button** - Navigate to Input Stack settings
- **Press INSTRUMENT button** - Navigate to Instrument settings
- **Press FX button** - Navigate to FX settings

**Behavior:**
- System bar shows current context (e.g., "Input", "Instrument", "FX", or "Menu")
- Content area shows menu structure or settings UI
- Bottom bar shows navigation hints

**Navigation:**
- **Joystick UP/DOWN**: Navigate menu items/settings list
- **Joystick RIGHT/Encoder rotate**: Enter submenu OR change parameter value
- **Joystick LEFT**: Go back/exit current level
- **Encoder rotate**: Change selected parameter value (real-time, no save needed)
- **Encoder press**: Toggle boolean/cycle enum parameter
- **INPUT button tap**: Toggle menu open/closed (returns to normal mode)

**Key Principle: Settings change in real-time**
- No "save" action needed - encoder rotation immediately updates values
- Changes take effect immediately
- Pressing menu buttons (INPUT/INSTRUMENT/FX) navigates to their top-level settings

**Menu Structure:**
```
Main Menu (tap INPUT):
├─ Track 1
│  ├─ Input Stack (press INPUT)
│  │  ├─ Chord Mapping
│  │  │  ├─ Key: C Major          ← Encoder rotates to change
│  │  │  ├─ Mode: Ionian          ← Encoder rotates to cycle
│  │  │  └─ Joystick Preset: Classic
│  │  └─ (other input plugins...)
│  ├─ Instrument (press INSTRUMENT)
│  │  └─ (instrument settings...)
│  └─ Effects (press FX)
│     └─ (effect settings...)
├─ Track 2
├─ Scene Management
├─ System Settings
│  ├─ MIDI Configuration
│  ├─ Display Settings
│  └─ Calibration
└─ Debug View
```

**Example Settings UI (when in Input Stack menu):**
```
┌─────────────────────────────────┐
│ 85%  Track 1           Input    │
├─────────────────────────────────┤
│ Chord Mapping                   │
│                                 │
│ > Key: C Major                  │
│   Mode: Ionian                  │
│   Joystick Preset: Classic      │
│                                 │
├─────────────────────────────────┤
│ ↑↓ nav  ◄► change  INPUT: back │
└─────────────────────────────────┘
```

**Navigation Flow Example:**
1. Normal mode → Tap INPUT → Opens main menu
2. Main menu → Press INPUT button → Enters "Input Stack" context
3. Input Stack → Joystick DOWN → Navigate to "Chord Mapping"
4. Chord Mapping → Joystick RIGHT → Enter Chord Mapping settings
5. Settings → Encoder rotate → Change "Key" value in real-time
6. Settings → INPUT tap → Go back one level (to Input Stack list)
7. Input Stack → INPUT tap → Go back to main menu
8. Main menu → INPUT tap → Close menu, return to normal mode

### 4. Plugin-Defined UI Mode

**Activation:**
- Plugin requests UI control (via interface)
- Can be activated from settings or automatically

**Behavior:**
- System bar still visible (always)
- Content area fully controlled by plugin
- Plugin can render custom graphics, menus, etc.

**Navigation:**
- Controlled by plugin (but should respect standard conventions)
- Plugin should handle INPUT button for exit/back

## Input Controls Mapping

### Normal Mode
- **Buttons 1-7**: Musical input (chords, notes, etc.)
- **Joystick**: Performance control (chord variations, etc.)
- **Encoder**: Usually inactive (or assigned to volume/filter)

### Menu Mode
- **INPUT button tap**: Toggle menu open/closed
- **INPUT button press (in menu)**: Navigate to Input Stack settings
- **INSTRUMENT button press (in menu)**: Navigate to Instrument settings  
- **FX button press (in menu)**: Navigate to FX settings
- **Joystick UP/DOWN**: Navigate list/options
- **Joystick LEFT**: Go back/exit current level
- **Joystick RIGHT**: Enter submenu/select item
- **Encoder rotate**: Change parameter value (real-time, immediate effect)
- **Encoder press**: Toggle/cycle (boolean/enum parameters)

### Record Button
- **Normal mode**: Record/playback control (track-specific)
- **Settings/Menu mode**: Cancel/discard changes (if applicable)
- **Hold with INPUT**: Toggle debug screen (existing behavior)

## Plugin Settings Interface

All plugins (Input, Instrument, Effect) can define settings that appear in Quick Settings or Menu mode.

### Settings Definition

Plugins implement:
```cpp
struct PluginSetting {
    const char* name;              // Display name
    SettingType type;              // INT, FLOAT, BOOL, ENUM, STRING
    void* value_ptr;               // Pointer to actual value
    float min_value;               // For numeric types
    float max_value;               // For numeric types
    const char** enum_options;     // For ENUM type (null-terminated array)
    void (*on_change)(void* value); // Optional callback
};

class IPluginWithSettings {
    virtual int GetSettingCount() = 0;
    virtual PluginSetting* GetSetting(int index) = 0;
    virtual void RenderSettingsUI(DisplayManager* display, int selected_index) = 0;
};
```

### Settings Rendering

Plugins can provide:
1. **Automatic rendering** - System generates list UI from `PluginSetting[]`
2. **Custom rendering** - Plugin provides `RenderSettingsUI()` for advanced UIs

## Implementation Plan

### Phase 1: Core UI System
1. **UI Manager** - Coordinates all UI components
   - Manages system bars
   - Coordinates content area rendering
   - Handles mode transitions

2. **System Bar Renderer** - Always-visible top bar
   - Battery percentage
   - Track name
   - Context indicator

3. **Content Area Manager** - Routes rendering to appropriate component
   - Normal mode → Plugin default view
   - Settings mode → Settings UI
   - Menu mode → Menu UI
   - Debug mode → Debug screen

### Phase 2: Menu System
1. **Menu Manager** - Hierarchical menu navigation
   - Menu tree structure
   - Breadcrumb navigation
   - Context-aware menu generation
   - Button-based navigation (INPUT/INSTRUMENT/FX buttons)

2. **Menu Mode** - Tap INPUT button to toggle
   - Main menu
   - Track settings
   - Input Stack / Instrument / FX navigation via buttons

### Phase 3: Settings System
1. **Settings Manager** - Handles plugin settings access
   - Query settings from plugins
   - Generate settings UI
   - Handle value changes (real-time, no save needed)

2. **Settings UI** - Integrated into menu system
   - Navigate to plugin via menu
   - Real-time parameter editing with encoder
   - Automatic list UI generation

### Phase 4: Plugin Integration
1. **Settings Interface** - `IPluginWithSettings`
   - Plugin registration
   - Settings querying
   - Custom UI support

2. **UI Adaptation** - Update existing UI components
   - MainUI → Content area renderer
   - DebugScreen → Content area renderer
   - Plugin UIs → Content area renderers

## File Structure

```
src/
  core/
    ui/
      ui_manager.h/cpp           # Main UI coordinator
      system_bar.h/cpp           # Top system bar renderer
      status_bar.h/cpp           # Bottom status bar renderer
      content_area.h/cpp         # Content area router
      settings_manager.h/cpp     # Plugin settings handling
      menu_manager.h/cpp         # Menu navigation system
      settings_ui.h/cpp          # Generic settings list UI
      main_ui.h/cpp              # (refactored) Default content view
      debug_screen.h/cpp         # (refactored) Debug content view
    plugin_interface.h           # Add IPluginWithSettings interface
```

## Migration Plan

### Step 1: Create Core UI System
- UI Manager class
- System Bar renderer
- Content Area manager
- Integrate with existing MainUI

### Step 2: Add Menu System
- Menu Manager
- Menu mode (tap INPUT to toggle)
- Button-based navigation (INPUT/INSTRUMENT/FX)
- Menu UI renderer
- Main menu structure

### Step 3: Add Settings System
- Settings Manager
- Settings UI integrated into menu
- Real-time parameter editing
- Add settings to ChordMappingInput plugin

### Step 4: Refactor Existing UI
- Move MainUI to content area renderer
- Move DebugScreen to content area renderer
- Remove duplicate display clearing/updating
- Unified rendering through UI Manager

### Step 5: Plugin Integration
- Add IPluginWithSettings interface
- Implement in ChordMappingInput
- Support custom UI rendering
- Document for plugin developers

## UX Flow Examples

### Example 1: Change Chord Mapping Key

**Normal Mode:**
1. User sees chord name: "Cmaj7"
2. System bar shows: "85%  Track 1      -"

**Tap INPUT:**
3. Enters Quick Settings mode
4. System bar shows: "85%  Track 1  Chord Mapping"
5. Content shows: Settings list with "Key: C Major" selected
6. Status bar shows: "↑↓ nav  ◄► change  INPUT: exit"

**Change Setting:**
7. Rotate encoder → Changes key (C → C# → D → ...)
8. Value updates in real-time

**Exit:**
9. Tap INPUT → Returns to normal mode
10. New chord reflects new key

### Example 2: Access Deep Menu

**Normal Mode:**
1. Hold INPUT button (500ms+)
2. Enters Menu Mode
3. Shows: Main menu with "Track 1 Settings" selected

**Navigate:**
4. Joystick DOWN → "Track 2 Settings"
5. Joystick RIGHT → Enters "Track 2 Settings"
6. Shows: "Input Stack", "Instrument", "Effects"
7. Joystick DOWN → "Input Stack"
8. Joystick RIGHT → Enters "Input Stack"
9. Shows: List of input plugins
10. Select "Chord Mapping" → Shows settings (same as Quick Settings)

**Exit:**
11. Joystick LEFT (multiple times) → Back to main menu
12. Tap INPUT → Exit to normal mode

## Design Decisions

### Why Tap vs Hold?
- **Tap (Quick Settings)**: Fast access to current plugin settings - most common case
- **Hold (Menu)**: Full system navigation - less frequent, shouldn't be accidental

### Why System Bars Always Visible?
- Context is critical - user always knows what track/plugin they're using
- Battery visibility prevents unexpected shutdowns
- Consistent UI frame reduces disorientation

### Why Plugin-Driven Content?
- Allows advanced UIs for complex plugins (e.g., sequencers with visual patterns)
- Simple plugins can use automatic list-based settings UI
- Flexibility for future plugin types

### Why Separate Settings from Menu?
- Quick Settings (tap) = fast tweaking during performance
- Menu (hold) = comprehensive system navigation
- Different use cases, different interaction patterns
