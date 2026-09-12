#pragma once

#include <cstdint>
#include <cstddef>

namespace OpenChord {

/**
 * Chord quality/types
 */
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

/**
 * Chord inversion
 */
enum class ChordInversion {
    ROOT,           // Root position
    FIRST,          // 1st inversion
    SECOND          // 2nd inversion
};

/**
 * Chord definition - represents a complete chord with all notes
 */
struct Chord {
    uint8_t root_note;          // MIDI note (0-127) for root
    ChordQuality quality;       // Chord type
    ChordInversion inversion;   // Inversion
    uint8_t notes[5];           // MIDI notes in the chord (max 5 for now)
    uint8_t note_count;         // Number of notes in chord (3-5)
    
    // For UI display
    char name[32];              // Human-readable name (e.g., "Cmaj7 1st Inv")
};

/**
 * Musical modes
 */
enum class MusicalMode {
    IONIAN = 0,      // Major
    DORIAN,
    PHRYGIAN,
    LYDIAN,
    MIXOLYDIAN,
    AEOLIAN,         // Natural Minor
    LOCRIAN,
    COUNT
};

/**
 * Musical key - root note (0-11 = C through B) + mode
 */
struct MusicalKey {
    uint8_t root_note;  // 0=C, 1=C#, 2=D, ..., 11=B
    MusicalMode mode;
    
    MusicalKey(uint8_t note = 0, MusicalMode m = MusicalMode::IONIAN) 
        : root_note(note), mode(m) {}
    
    bool operator==(const MusicalKey& other) const {
        return root_note == other.root_note && mode == other.mode;
    }
};

// JoystickDirection is defined in joystick_input_handler.h
// We need to include it here for JoystickPreset array declaration
// Include at the end to minimize dependencies
// Forward declare for now - full include in cpp

// Forward declare JoystickDirection - full definition in cpp
namespace OpenChord {
    enum class JoystickDirection;
}

/**
 * Joystick direction preset - defines what chord variation each direction applies
 */
struct JoystickPreset {
    const char* name;                              // Preset name
    ChordQuality direction_qualities[8];           // Chord quality for each of 8 directions
    int direction_modifications[8];                // Semitone modifications or special flags
    // Future: could add voicing styles, note substitutions, etc.
};

/**
 * Chord Engine - Chord theory and generation
 * 
 * Provides chord definitions, note generation, and preset management
 */
class ChordEngine {
public:
    ChordEngine();
    ~ChordEngine();
    
    /**
     * Generate chord notes from root and quality
     * Returns the number of notes generated
     */
    void GenerateChord(Chord* chord, uint8_t root_midi_note, ChordQuality quality, ChordInversion inversion);
    
    /**
     * Get human-readable chord name
     * Populates chord->name with string like "Cmaj7 1st Inv"
     */
    void GetChordName(Chord* chord);
    
    /**
     * Get scale degree for a button
     * Buttons are arranged physically like a piano: White0, Black4, White1, Black5, White2, Black6, White3
     * Left-to-right physical order: 0, 4, 1, 5, 2, 6, 3 → maps to scale degrees I, II, III, IV, V, VI, VII
     * Returns the MIDI root note for that scale degree
     */
    void GetButtonMapping(MusicalKey key, int button_index, uint8_t* root_midi_note) const;
    
    /**
     * Convert physical button index to scale degree index (0-6)
     * Physical order: 0, 4, 1, 5, 2, 6, 3 → Scale degrees: 0, 1, 2, 3, 4, 5, 6
     */
    int PhysicalButtonToScaleDegree(int physical_button_index) const;
    
    /**
     * Get chord quality for a scale degree in a mode
     * Scale degree is 0-6 (representing I-VII)
     */
    ChordQuality GetChordQualityForDegree(MusicalMode mode, int scale_degree) const;
    
    /**
     * Get scale intervals for a mode (semitones from root)
     */
    void GetScaleIntervals(MusicalMode mode, int* intervals, int* count) const;
    
    /**
     * Apply joystick direction variation to a chord
     * Returns the modified chord quality
     * Note: direction is passed as int to avoid circular dependency, cast internally
     */
    ChordQuality ApplyJoystickVariation(ChordQuality base_quality, int direction, int preset_index) const;
    
    /**
     * Get joystick preset by index
     */
    const JoystickPreset* GetJoystickPreset(int index) const;
    
    /**
     * Get number of available joystick presets
     */
    int GetJoystickPresetCount() const;
    
    /**
     * Convert MIDI note to note name (C, C#, D, etc.)
     */
    static void GetNoteName(uint8_t midi_note, char* buffer, size_t buffer_size);
    
private:
    /**
     * Get interval in semitones for chord quality
     * Returns array of intervals and count
     */
    void GetIntervalsForQuality(ChordQuality quality, int* intervals, int* count) const;
    
    /**
     * Apply inversion to chord notes
     */
    void ApplyInversion(uint8_t* notes, uint8_t note_count, ChordInversion inversion);
    
};

} // namespace OpenChord

