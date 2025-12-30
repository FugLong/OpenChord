# Chord Mapping Implementation Plan

[← Back to Documentation](../README.md)

## Overview

This document outlines the implementation plan for the primary chord mapping use case. The chord mapping system allows users to play chords using buttons 1-7 and modify them with the joystick (inversions, extensions).

## Architecture Alignment

**Key Principle**: Chord mapping is implemented as an **Input Plugin** (IInputPlugin), NOT a PlayMode, following the firmware architecture specification:
- "Features like chord/note input mapping should be implemented as input plugins, not PlayModes."
- Lives in the Track's Input Stack
- Generates/transforms MIDI from button presses and joystick
- Works seamlessly with existing Track/Instrument/FX pipeline

## Components

### 1. Chord Mapping Input Plugin (`chord_mapping_input.h/cpp`)
**Location**: `src/plugins/input/chord_mapping_input.h/cpp`

**Features**:
- Implements `IInputPlugin` interface
- Maps buttons 1-7 (MusicalButton::WHITE_0 through BLACK_2) to scale degrees (I-VII) of the current key/mode
- **8-Directional Joystick Presets**: Joystick directions (UP, DOWN, LEFT, RIGHT, and diagonals) apply smart chord variations from configurable presets
- **Musical Key/Mode Support**: Buttons map to diatonic chords based on selected key root and mode (Ionian, Dorian, Phrygian, etc.)
- **Joystick Preset System**: Multiple presets (Classic, Jazzy, Ambient) with 8-directional chord quality mappings
- Generates MIDI NOTE_ON/OFF events for all notes in the chord
- Handles button press/release events to trigger chord on/off
- Real-time chord updates when joystick direction changes while button is held

### 2. Chord Theory Library (`chord_engine.h/cpp`)
**Location**: `src/core/music/chord_engine.h/cpp`

**Features**:
- **Chord Definitions**:
  - Basic qualities: Major, minor, diminished, augmented
  - Extensions: 7th (maj7, min7, dom7), 9th (maj9, min9, dom9), sus4, sus2
  - Inversions: Root position, 1st inversion, 2nd inversion
- **Chord Name Generation**: String generation for UI display (e.g., "Cmaj7")
- **Musical Key and Mode Support**: 
  - 12 root notes (C through B)
  - 7 modes (Ionian/Major, Dorian, Phrygian, Lydian, Mixolydian, Aeolian/Minor, Locrian)
  - Automatic diatonic chord quality calculation per scale degree
- **Joystick Preset System**: Predefined 8-directional chord variation presets (Classic, Jazzy, Ambient)

**Chord Structure**:
```cpp
enum class ChordQuality {
    MAJOR,          // 1-3-5
    MINOR,          // 1-b3-5
    DIMINISHED,     // 1-b3-b5
    AUGMENTED,      // 1-3-#5
    MAJOR_7,        // 1-3-5-7
    MINOR_7,        // 1-b3-5-b7
    DOMINANT_7,     // 1-3-5-b7
    MAJOR_9,        // 1-3-5-7-9
    MINOR_9,        // 1-b3-5-b7-9
    SUS4,           // 1-4-5
    SUS2            // 1-2-5
};

enum class ChordInversion {
    ROOT,           // Root position
    FIRST,          // 1st inversion
    SECOND          // 2nd inversion
};
```

### 3. Main UI System (`main_ui.h/cpp`)
**Location**: `src/core/ui/main_ui.h/cpp`

**Features**:
- Default view: Displays current chord name (e.g., "Cmaj7")
- Displays current musical key and mode (e.g., "C Maj")
- Displays active joystick preset name (e.g., "Preset: Classic")
- Updates in real-time when chord changes
- Replaces debug screen as the default view
- Simple, clean display focused on chord information

### 4. Debug Screen Toggle
**Enhancement to existing**: `src/core/ui/debug_screen.h/cpp`

**Features**:
- Button combo: INPUT + RECORD (held for ~500ms) toggles debug screen
- Debug screen disabled by default (`DEBUG_SCREEN_ENABLED` compile flag)
- Toggle state persists during runtime
- Easy to access when needed, hidden during normal use

## Integration Points

### Track System Integration
1. Create Track instance in `main.cpp`
2. Add `ChordMappingInput` plugin to track's input stack
3. Track processes input → generates MIDI → sends to Instrument
4. MIDI Output: Route Track-generated MIDI to `midi_handler` for USB and TRS

### MIDI Routing Flow
```
Button Press → ChordMappingInput Plugin → Generate MIDI Events
    ↓
Track Collects Events from Input Stack
    ↓
Track Processes through Instrument (if present)
    ↓
main.cpp Routes Track MIDI to midi_handler.SendMidi()
    ↓
MIDI sent to USB MIDI and TRS MIDI outputs
```

### Existing Systems Used
- **InputManager**: Access to buttons and joystick
- **DisplayManager**: Render chord name on screen
- **MidiHandler**: Send MIDI to USB and TRS
- **Track System**: Process MIDI through input stack
- **IInputPlugin Interface**: Plugin integration

## Configuration (Preset System)

### Preset Structure
```cpp
struct ChordPreset {
    const char* name;                  // Display name
    uint8_t root_notes[7];             // MIDI note for each button (1-7)
    ChordQuality qualities[7];         // Chord quality for each button
    uint8_t base_octave;               // Base octave for chords (default: 4 = C4)
};
```

### Button Mapping (Scale Degree Based)
Buttons 1-7 map to scale degrees I-VII of the currently selected key/mode:

**Default Key: C Ionian (C Major)**
- Button 1 → Scale degree I → C Major
- Button 2 → Scale degree II → D Minor  
- Button 3 → Scale degree III → E Minor
- Button 4 → Scale degree IV → F Major
- Button 5 → Scale degree V → G Major
- Button 6 → Scale degree VI → A Minor
- Button 7 → Scale degree VII → B Diminished

**Example: D Dorian**
- Button 1 → D Minor (I in Dorian)
- Button 2 → E Minor (II in Dorian)
- Button 3 → F Major (III in Dorian)
- ... (chord qualities automatically determined by mode)

### Joystick Preset System
Each preset defines chord variations for 8 joystick directions:
- **Classic**: Traditional extensions (Major 9th, Minor 7th, Dominant 7th, etc.)
- **Jazzy**: Jazz-oriented voicings and extensions
- **Ambient**: Open, ethereal sounds (Sus2, Sus4, extended chords)

When a button is held and joystick is moved, the chord quality changes based on the preset's direction mapping.

## UI Flow

### Default State (Normal Operation)
- Display shows current chord name: "Cmaj7 1st Inv"
- Updates automatically when chord changes
- Clean, minimal interface

### Debug Mode (Toggled via Button Combo)
- Hold INPUT + RECORD for ~500ms → Debug screen appears
- Debug screen shows system status, inputs, analog, audio, MIDI
- Same button combo to exit debug mode
- Debug screen respects existing navigation (encoder)

## File Structure

```
src/
  core/
    music/
      chord_engine.h/cpp          # Chord theory, definitions, presets
    ui/
      main_ui.h/cpp               # Default chord name display
      debug_screen.h/cpp          # (existing, add toggle support)
  plugins/
    input/
      chord_mapping_input.h/cpp   # Main chord mapping plugin
  main.cpp                        # Integration & system wiring
```

## Implementation Order

1. **Chord Theory Library** (Foundation)
   - Chord definitions and structures
   - Chord name generation
   - Preset configurations

2. **Chord Mapping Plugin** (Core Functionality)
   - Plugin implementation following IInputPlugin
   - Button mapping to chords
   - Joystick control for variations
   - MIDI event generation

3. **Main UI** (Display System)
   - Chord name rendering
   - Update logic

4. **Debug Toggle** (UX Enhancement)
   - Button combo detection
   - Toggle state management

5. **Track Integration** (System Wiring)
   - Track creation and configuration
   - Plugin installation
   - MIDI routing

6. **Main.cpp Updates** (System Integration)
   - Remove old direct MIDI generation code
   - Wire new Track-based system
   - Update UI rendering loop

7. **Testing** (Verification)
   - Verify MIDI output on USB
   - Verify MIDI output on TRS
   - Test chord generation accuracy
   - Test joystick variations

## Design Decisions

### Smart Defaults
- Default key: C Ionian (C Major)
- Default mode: Ionian (Major scale)
- Default joystick preset: "Classic" (preset index 0)
- Buttons 1-7 map to scale degrees I-VII, automatically calculating diatonic chord qualities based on the selected mode
- Base octave: C4 (MIDI note 60) for root note calculation
- Chord qualities are mode-aware (e.g., in Dorian mode, degree I is minor, not major)

### Joystick Mapping (8-Directional Presets)
The joystick uses 8-directional detection with configurable presets:

- **8 Directions**: UP, DOWN, LEFT, RIGHT, UP_LEFT, UP_RIGHT, DOWN_LEFT, DOWN_RIGHT
- **CENTER**: No modification (uses base diatonic chord quality)
- **Each Direction**: Maps to a chord quality defined in the active preset
- **Dynamic Updates**: When joystick direction changes while a button is held, the chord updates in real-time

**Dead Zone**: 0.3 threshold to prevent accidental direction changes near center

**Preset Examples**:
- **Classic Preset** (default):
  - UP → Major 9th
  - UP_RIGHT → Major 7th
  - RIGHT → Dominant 7th
  - DOWN_RIGHT → Sus4
  - DOWN → Minor 7th
  - DOWN_LEFT → Minor 9th
  - LEFT → Major 9th
  - UP_LEFT → Sus2

### Velocity
- Fixed velocity for now (100 = mezzo-forte)
- Can be enhanced later with pressure-sensitive input or encoder control

### Presets
- Hardcoded presets initially
- Can be expanded later to load from storage/configuration files
- Easy to add more presets (jazz voicings, modal scales, etc.)

## Architecture Compliance

### ✅ Follows Plugin Interface
- Implements `IInputPlugin` correctly
- Provides `GenerateMIDI()` for MIDI event generation
- Handles `ProcessMIDI()` for MIDI transformation (passthrough)
- Implements all required `IPlugin` methods

### ✅ Uses Existing Track System
- Plugins live in Track's Input Stack
- Track processes MIDI through standard pipeline
- No shortcuts or bypassing architecture

### ✅ Respects Input Stack Architecture
- Input plugins generate/transform MIDI
- Multiple plugins can be stacked
- Priority-based processing

### ✅ UI Follows Existing Patterns
- Uses DisplayManager for rendering
- Follows existing display update patterns
- Minimal, focused UI

### ✅ MIDI Uses Existing Handler
- No custom MIDI routing
- Uses `midi_handler.SendMidi()` for output
- Supports both USB and TRS automatically

### ✅ No Bespoke Code
- All code follows established patterns
- Reusable components
- Maintainable structure
- No code that will need rewriting when UX system is added

## Future Enhancements

1. **User-Defined Presets**: Store/load presets from SD card
2. **Chord Voicing Options**: Different voicings (close, open, drop 2, etc.)
3. **Velocity Control**: Dynamic velocity from encoder or pressure
4. **Scale Modes**: Different scale mappings (pentatonic, modes, etc.)
5. **Arpeggiator Integration**: Add arpeggio patterns over chords
6. **Chord Progression Memory**: Store and recall chord progressions

