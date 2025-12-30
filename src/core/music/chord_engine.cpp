#include "chord_engine.h"
#include "../io/joystick_input_handler.h"  // For JoystickDirection enum
#include <cstring>
#include <cstdio>

namespace OpenChord {

// Scale intervals for each mode (semitones from root)
// Ionian (Major):     0, 2, 4, 5, 7, 9, 11
// Dorian:             0, 2, 3, 5, 7, 9, 10
// Phrygian:           0, 1, 3, 5, 7, 8, 10
// Lydian:             0, 2, 4, 6, 7, 9, 11
// Mixolydian:         0, 2, 4, 5, 7, 9, 10
// Aeolian (Minor):    0, 2, 3, 5, 7, 8, 10
// Locrian:            0, 1, 3, 5, 6, 8, 10

static const int mode_intervals[7][7] = {
    {0, 2, 4, 5, 7, 9, 11},   // Ionian (Major)
    {0, 2, 3, 5, 7, 9, 10},   // Dorian
    {0, 1, 3, 5, 7, 8, 10},   // Phrygian
    {0, 2, 4, 6, 7, 9, 11},   // Lydian
    {0, 2, 4, 5, 7, 9, 10},   // Mixolydian
    {0, 2, 3, 5, 7, 8, 10},   // Aeolian (Natural Minor)
    {0, 1, 3, 5, 6, 8, 10}    // Locrian
};

// Diatonic chord qualities for each mode (I, II, III, IV, V, VI, VII)
static const ChordQuality mode_chord_qualities[7][7] = {
    // Ionian (Major): I, ii, iii, IV, V, vi, vii°
    {ChordQuality::MAJOR, ChordQuality::MINOR, ChordQuality::MINOR, ChordQuality::MAJOR, 
     ChordQuality::MAJOR, ChordQuality::MINOR, ChordQuality::DIMINISHED},
    
    // Dorian: i, ii, III, IV, v, vi°, VII
    {ChordQuality::MINOR, ChordQuality::MINOR, ChordQuality::MAJOR, ChordQuality::MAJOR,
     ChordQuality::MINOR, ChordQuality::DIMINISHED, ChordQuality::MAJOR},
    
    // Phrygian: i, II, III, iv, v°, VI, vii
    {ChordQuality::MINOR, ChordQuality::MAJOR, ChordQuality::MAJOR, ChordQuality::MINOR,
     ChordQuality::DIMINISHED, ChordQuality::MAJOR, ChordQuality::MINOR},
    
    // Lydian: I, II, iii, iv°, V, vi, vii
    {ChordQuality::MAJOR, ChordQuality::MAJOR, ChordQuality::MINOR, ChordQuality::DIMINISHED,
     ChordQuality::MAJOR, ChordQuality::MINOR, ChordQuality::MINOR},
    
    // Mixolydian: I, ii, iii°, IV, v, vi, VII
    {ChordQuality::MAJOR, ChordQuality::MINOR, ChordQuality::DIMINISHED, ChordQuality::MAJOR,
     ChordQuality::MINOR, ChordQuality::MINOR, ChordQuality::MAJOR},
    
    // Aeolian (Minor): i, ii°, III, iv, v, VI, VII
    {ChordQuality::MINOR, ChordQuality::DIMINISHED, ChordQuality::MAJOR, ChordQuality::MINOR,
     ChordQuality::MINOR, ChordQuality::MAJOR, ChordQuality::MAJOR},
    
    // Locrian: i°, II, iii, iv, V, VI, vii
    {ChordQuality::DIMINISHED, ChordQuality::MAJOR, ChordQuality::MINOR, ChordQuality::MINOR,
     ChordQuality::MAJOR, ChordQuality::MAJOR, ChordQuality::MINOR}
};

// Joystick presets - musically organized chord variations
// 
// Layout philosophy (circle of directions):
//   - CARDINAL directions (UP/DOWN/LEFT/RIGHT): Most common, foundational variations
//   - DIAGONAL directions: More exotic, experimental variations
//
// Musical logic (circle layout):
//   UP (NORTH):     Bright, major extensions (maj9, maj7)
//   DOWN (SOUTH):   Dark, minor extensions (m7, m9)
//   RIGHT (EAST):   Forward motion, tense (7, dom7)
//   LEFT (WEST):    Suspended, ambiguous (sus4, sus2)
//   Diagonals:      Blend of adjacent cardinals for unique colors
//     NE (UP_RIGHT):   Bright + tense (maj7, aug)
//     SE (DOWN_RIGHT): Dark + tense (m9, dim)
//     SW (DOWN_LEFT):  Dark + suspended (sus2, dim)
//     NW (UP_LEFT):    Bright + suspended (sus2, aug)
//
// Array order: [NORTH, NORTHEAST, EAST, SOUTHEAST, SOUTH, SOUTHWEST, WEST, NORTHWEST]
//              [UP,    UP_RIGHT,  RIGHT, DOWN_RIGHT, DOWN, DOWN_LEFT, LEFT, UP_LEFT  ]
//
// Each preset must have 8 UNIQUE chord qualities - no duplicates!
// To add a new preset, add a new entry to this array following the musical logic above.

static const JoystickPreset joystick_presets[] = {
    // Preset 0: "Classic" - traditional, well-balanced chord palette
    // Cardinals: Basic 7ths and 9ths | Diagonals: Suspended and extended variations
    {
        "Classic",
        {
            ChordQuality::MAJOR_9,    // UP:    Major 9th (bright, open, jazzy)
            ChordQuality::MAJOR_7,    // NE:    Major 7th (bright + forward = smooth)
            ChordQuality::DOMINANT_7, // RIGHT: Dominant 7th (tense, forward motion)
            ChordQuality::MINOR_9,    // SE:    Minor 9th (dark + tense = melancholic)
            ChordQuality::MINOR_7,    // DOWN:  Minor 7th (dark, smooth)
            ChordQuality::SUS2,       // SW:    Sus2 (dark + suspended = mysterious)
            ChordQuality::SUS4,       // LEFT:  Sus4 (suspended, ambiguous)
            ChordQuality::AUGMENTED   // NW:    Augmented (bright + suspended = dreamy)
        },
        {0, 0, 0, 0, 0, 0, 0, 0}
    },
    
    // Preset 1: "Jazzy" - extended harmonies for jazz voice leading
    // Cardinals: 9ths | Diagonals: 7ths and altered tones
    {
        "Jazzy",
        {
            ChordQuality::MAJOR_9,    // UP:    Major 9th (bright, rich)
            ChordQuality::MAJOR_7,    // NE:    Major 7th (smooth jazz)
            ChordQuality::DOMINANT_7, // RIGHT: Dominant 7th (bluesy, functional)
            ChordQuality::MINOR_9,    // SE:    Minor 9th (dark jazz)
            ChordQuality::MINOR_7,    // DOWN:  Minor 7th (dark, classic)
            ChordQuality::DIMINISHED, // SW:    Diminished (dark + tense = altered)
            ChordQuality::SUS4,       // LEFT:  Sus4 (suspended, colorful)
            ChordQuality::AUGMENTED   // NW:    Augmented (altered, mysterious)
        },
        {0, 0, 0, 0, 0, 0, 0, 0}
    },
    
    // Preset 2: "Ambient" - open, ethereal, minimal
    // Cardinals: Suspended chords | Diagonals: Minimal extensions
    {
        "Ambient",
        {
            ChordQuality::SUS2,       // UP:    Sus2 (open, airy, bright)
            ChordQuality::MAJOR_7,    // NE:    Major 7th (bright + smooth)
            ChordQuality::SUS4,       // RIGHT: Sus4 (suspended, floating)
            ChordQuality::MINOR_7,    // SE:    Minor 7th (dark + smooth)
            ChordQuality::MINOR_9,    // DOWN:  Minor 9th (dark, extended)
            ChordQuality::DIMINISHED, // SW:    Diminished (dark + tense = dissonant)
            ChordQuality::AUGMENTED,  // LEFT:  Augmented (suspended + altered)
            ChordQuality::MAJOR_9     // NW:    Major 9th (bright + open)
        },
        {0, 0, 0, 0, 0, 0, 0, 0}
    },
    
    // Preset 3: "Functional" - traditional harmony with clear voice leading
    // Cardinals: Strong functions | Diagonals: Color tones
    {
        "Functional",
        {
            ChordQuality::MAJOR_7,    // UP:    Major 7th (bright, stable)
            ChordQuality::MAJOR_9,    // NE:    Major 9th (bright extension)
            ChordQuality::DOMINANT_7, // RIGHT: Dominant 7th (tense, resolves)
            ChordQuality::MINOR_7,    // SE:    Minor 7th (dark, smooth)
            ChordQuality::MINOR_9,    // DOWN:  Minor 9th (dark, extended)
            ChordQuality::DIMINISHED, // SW:    Diminished (dark + tense)
            ChordQuality::SUS4,       // LEFT:  Sus4 (suspended, pre-dominant)
            ChordQuality::SUS2        // NW:    Sus2 (bright, open)
        },
        {0, 0, 0, 0, 0, 0, 0, 0}
    }
};

ChordEngine::ChordEngine() {
}

ChordEngine::~ChordEngine() {
}

void ChordEngine::GetScaleIntervals(MusicalMode mode, int* intervals, int* count) const {
    *count = 7;
    int mode_idx = static_cast<int>(mode);
    if (mode_idx >= 0 && mode_idx < 7) {
        for (int i = 0; i < 7; i++) {
            intervals[i] = mode_intervals[mode_idx][i];
        }
    } else {
        // Default to Ionian (major)
        for (int i = 0; i < 7; i++) {
            intervals[i] = mode_intervals[0][i];
        }
    }
}

int ChordEngine::PhysicalButtonToScaleDegree(int physical_button_index) const {
    // Physical layout (left to right): White0, Black4, White1, Black5, White2, Black6, White3
    // This maps to scale degrees: I, II, III, IV, V, VI, VII (0, 1, 2, 3, 4, 5, 6)
    // Physical left-to-right: 0, 4, 1, 5, 2, 6, 3 → Scale degrees: 0, 1, 2, 3, 4, 5, 6
    switch (physical_button_index) {
        case 0: return 0;  // Leftmost white → I
        case 4: return 1;  // First black → II
        case 1: return 2;  // Second white → III
        case 5: return 3;  // Second black → IV
        case 2: return 4;  // Third white → V
        case 6: return 5;  // Third black → VI
        case 3: return 6;  // Rightmost white → VII
        default: return 0;
    }
}

void ChordEngine::GetButtonMapping(MusicalKey key, int button_index, uint8_t* root_midi_note) const {
    if (!root_midi_note || button_index < 0 || button_index >= 7) {
        return;
    }
    
    // Convert physical button index to scale degree
    int scale_degree = PhysicalButtonToScaleDegree(button_index);
    
    // Get scale intervals for the mode
    int intervals[7];
    int count;
    GetScaleIntervals(key.mode, intervals, &count);
    
    // Calculate MIDI note: root (C4=60) + root note offset + scale interval
    // C4 = 60 (calculated directly, no need for separate base_octave variable)
    uint8_t root_midi_base = 60 + key.root_note;  // Root note in MIDI (60-71)
    
    // Get interval for this scale degree
    int interval = intervals[scale_degree];
    
    // Calculate final MIDI note
    *root_midi_note = root_midi_base + interval;
    
    // Wrap if needed (keep in reasonable range)
    if (*root_midi_note > 127) {
        *root_midi_note = 127;
    }
}

ChordQuality ChordEngine::GetChordQualityForDegree(MusicalMode mode, int scale_degree) const {
    if (scale_degree < 0 || scale_degree >= 7) {
        return ChordQuality::MAJOR;  // Default
    }
    
    int mode_idx = static_cast<int>(mode);
    if (mode_idx >= 0 && mode_idx < 7) {
        return mode_chord_qualities[mode_idx][scale_degree];
    }
    
    // Default to Ionian (major) if invalid mode
    return mode_chord_qualities[0][scale_degree];
}

ChordQuality ChordEngine::ApplyJoystickVariation(ChordQuality base_quality, int direction, int preset_index) const {
    // JoystickDirection enum: CENTER=0, UP=1, DOWN=2, LEFT=3, RIGHT=4, UP_LEFT=5, UP_RIGHT=6, DOWN_LEFT=7, DOWN_RIGHT=8
    // Preset array order: [NORTH, NORTHEAST, EAST, SOUTHEAST, SOUTH, SOUTHWEST, WEST, NORTHWEST]
    //                     [  0  ,     1     ,  2  ,    3     ,  4  ,     5    ,  6  ,    7    ]
    // We need to map enum values to preset array indices:
    // UP(1) -> 0, UP_RIGHT(6) -> 1, RIGHT(4) -> 2, DOWN_RIGHT(8) -> 3,
    // DOWN(2) -> 4, DOWN_LEFT(7) -> 5, LEFT(3) -> 6, UP_LEFT(5) -> 7
    
    if (direction <= 0 || direction > 8) {
        return base_quality;  // No change for center or invalid
    }
    
    if (preset_index < 0 || preset_index >= GetJoystickPresetCount()) {
        preset_index = 0;  // Default to first preset
    }
    
    const JoystickPreset* preset = GetJoystickPreset(preset_index);
    if (!preset) {
        return base_quality;
    }
    
    // Map JoystickDirection enum to preset array index
    // Preset order: NORTH, NORTHEAST, EAST, SOUTHEAST, SOUTH, SOUTHWEST, WEST, NORTHWEST
    int dir_index = -1;
    switch (direction) {
        case 1: dir_index = 0; break;  // UP -> NORTH
        case 6: dir_index = 1; break;  // UP_RIGHT -> NORTHEAST
        case 4: dir_index = 2; break;  // RIGHT -> EAST
        case 8: dir_index = 3; break;  // DOWN_RIGHT -> SOUTHEAST
        case 2: dir_index = 4; break;  // DOWN -> SOUTH
        case 7: dir_index = 5; break;  // DOWN_LEFT -> SOUTHWEST
        case 3: dir_index = 6; break;  // LEFT -> WEST
        case 5: dir_index = 7; break;  // UP_LEFT -> NORTHWEST
        default: return base_quality;
    }
    
    if (dir_index < 0 || dir_index >= 8) {
        return base_quality;
    }
    
    ChordQuality variation = preset->direction_qualities[dir_index];
    
    // Smart application: if base is minor and variation is major-only, we could blend
    // For now, just return the variation quality, but preserve base quality type
    // This means if base is minor, we try to keep it minor if variation allows
    
    // If variation is a 7th/9th extension, we can apply it to any base
    // If variation is major/minor, we need to be smarter
    
    // For extensions (7th, 9th), apply them to the base quality
    if (variation == ChordQuality::MAJOR_7 || variation == ChordQuality::MINOR_7 ||
        variation == ChordQuality::DOMINANT_7 || variation == ChordQuality::MAJOR_9 ||
        variation == ChordQuality::MINOR_9) {
        // Extensions can be applied to any base
        return variation;
    }
    
    // For sus chords, apply them
    if (variation == ChordQuality::SUS2 || variation == ChordQuality::SUS4) {
        return variation;
    }
    
    // For basic major/minor, preserve the base quality type but add extensions if needed
    // Actually, just return the variation - let the preset designer choose
    
    // IMPORTANT: This variation system works with ANY key and mode!
    // The root note comes from key.root_note (C=0...B=11) + mode scale intervals.
    // Presets define chord QUALITIES (maj7, min9, etc.), not specific notes.
    // Example: C Major button 0 + UP (maj9) = Cmaj9, D Dorian button 0 + UP (maj9) = Dmaj9
    
    return variation;
}

const JoystickPreset* ChordEngine::GetJoystickPreset(int index) const {
    if (index >= 0 && index < GetJoystickPresetCount()) {
        return &joystick_presets[index];
    }
    return nullptr;
}

int ChordEngine::GetJoystickPresetCount() const {
    return sizeof(joystick_presets) / sizeof(joystick_presets[0]);
}

void ChordEngine::GenerateChord(Chord* chord, uint8_t root_midi_note, ChordQuality quality, ChordInversion inversion) {
    if (!chord) return;
    
    chord->root_note = root_midi_note;
    chord->quality = quality;
    chord->inversion = inversion;
    
    // Get intervals for this quality
    int intervals[5];
    int interval_count = 0;
    GetIntervalsForQuality(quality, intervals, &interval_count);
    
    // Generate base chord notes
    uint8_t notes[5];
    notes[0] = root_midi_note;  // Root
    
    for (int i = 0; i < interval_count; i++) {
        uint8_t note = root_midi_note + intervals[i];
        if (note > 127) note = 127;  // Clamp to MIDI range
        notes[i + 1] = note;
    }
    
    chord->note_count = interval_count + 1;
    
    // Copy to chord structure
    for (int i = 0; i < chord->note_count; i++) {
        chord->notes[i] = notes[i];
    }
    
    // Apply inversion
    if (inversion != ChordInversion::ROOT) {
        ApplyInversion(chord->notes, chord->note_count, inversion);
    }
    
    // Generate name
    GetChordName(chord);
}

void ChordEngine::GetIntervalsForQuality(ChordQuality quality, int* intervals, int* count) const {
    *count = 0;
    
    switch (quality) {
        case ChordQuality::MAJOR:
            intervals[0] = 4;   // Major 3rd
            intervals[1] = 7;   // Perfect 5th
            *count = 2;
            break;
            
        case ChordQuality::MINOR:
            intervals[0] = 3;   // Minor 3rd
            intervals[1] = 7;   // Perfect 5th
            *count = 2;
            break;
            
        case ChordQuality::DIMINISHED:
            intervals[0] = 3;   // Minor 3rd
            intervals[1] = 6;   // Diminished 5th
            *count = 2;
            break;
            
        case ChordQuality::AUGMENTED:
            intervals[0] = 4;   // Major 3rd
            intervals[1] = 8;   // Augmented 5th
            *count = 2;
            break;
            
        case ChordQuality::MAJOR_7:
            intervals[0] = 4;   // Major 3rd
            intervals[1] = 7;   // Perfect 5th
            intervals[2] = 11;  // Major 7th
            *count = 3;
            break;
            
        case ChordQuality::MINOR_7:
            intervals[0] = 3;   // Minor 3rd
            intervals[1] = 7;   // Perfect 5th
            intervals[2] = 10;  // Minor 7th
            *count = 3;
            break;
            
        case ChordQuality::DOMINANT_7:
            intervals[0] = 4;   // Major 3rd
            intervals[1] = 7;   // Perfect 5th
            intervals[2] = 10;  // Minor 7th
            *count = 3;
            break;
            
        case ChordQuality::MAJOR_9:
            intervals[0] = 4;   // Major 3rd
            intervals[1] = 7;   // Perfect 5th
            intervals[2] = 11;  // Major 7th
            intervals[3] = 14;  // Major 9th (octave + 2 semitones)
            *count = 4;
            break;
            
        case ChordQuality::MINOR_9:
            intervals[0] = 3;   // Minor 3rd
            intervals[1] = 7;   // Perfect 5th
            intervals[2] = 10;  // Minor 7th
            intervals[3] = 14;  // Major 9th
            *count = 4;
            break;
            
        case ChordQuality::SUS4:
            intervals[0] = 5;   // Perfect 4th
            intervals[1] = 7;   // Perfect 5th
            *count = 2;
            break;
            
        case ChordQuality::SUS2:
            intervals[0] = 2;   // Major 2nd
            intervals[1] = 7;   // Perfect 5th
            *count = 2;
            break;
    }
}

void ChordEngine::ApplyInversion(uint8_t* notes, uint8_t note_count, ChordInversion inversion) {
    if (!notes || note_count == 0) return;
    
    switch (inversion) {
        case ChordInversion::ROOT:
            // No change
            break;
            
        case ChordInversion::FIRST: {
            // Move root up one octave
            uint8_t root = notes[0];
            for (int i = 0; i < note_count - 1; i++) {
                notes[i] = notes[i + 1];
            }
            notes[note_count - 1] = root + 12;  // Root up one octave
            break;
        }
        
        case ChordInversion::SECOND: {
            // Move root and 3rd up one octave
            if (note_count >= 3) {
                uint8_t root = notes[0];
                uint8_t third = notes[1];
                
                // Shift remaining notes down
                notes[0] = notes[2];
                if (note_count > 3) {
                    for (int i = 1; i < note_count - 2; i++) {
                        notes[i] = notes[i + 2];
                    }
                }
                
                // Add inverted notes
                notes[note_count - 2] = root + 12;
                notes[note_count - 1] = third + 12;
            }
            break;
        }
    }
}

void ChordEngine::GetChordName(Chord* chord) {
    if (!chord) return;
    
    // Get note name without octave for cleaner display
    const char* note_names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int note = chord->root_note % 12;
    const char* note_name = note_names[note];
    
    const char* quality_str = "";
    switch (chord->quality) {
        case ChordQuality::MAJOR: quality_str = ""; break;
        case ChordQuality::MINOR: quality_str = "m"; break;
        case ChordQuality::DIMINISHED: quality_str = "dim"; break;
        case ChordQuality::AUGMENTED: quality_str = "aug"; break;
        case ChordQuality::MAJOR_7: quality_str = "maj7"; break;
        case ChordQuality::MINOR_7: quality_str = "m7"; break;
        case ChordQuality::DOMINANT_7: quality_str = "7"; break;
        case ChordQuality::MAJOR_9: quality_str = "maj9"; break;
        case ChordQuality::MINOR_9: quality_str = "m9"; break;
        case ChordQuality::SUS4: quality_str = "sus4"; break;
        case ChordQuality::SUS2: quality_str = "sus2"; break;
    }
    
    const char* inversion_str = "";
    switch (chord->inversion) {
        case ChordInversion::ROOT: inversion_str = ""; break;
        case ChordInversion::FIRST: inversion_str = " 1st Inv"; break;
        case ChordInversion::SECOND: inversion_str = " 2nd Inv"; break;
    }
    
    snprintf(chord->name, sizeof(chord->name), "%s%s%s", note_name, quality_str, inversion_str);
}

void ChordEngine::GetNoteName(uint8_t midi_note, char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    
    const char* note_names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    int note = midi_note % 12;
    int octave = (midi_note / 12) - 1;
    
    // Use flat names for some sharps (optional enhancement)
    if (note == 1) snprintf(buffer, buffer_size, "Db%d", octave);      // C# = Db
    else if (note == 3) snprintf(buffer, buffer_size, "Eb%d", octave); // D# = Eb
    else if (note == 6) snprintf(buffer, buffer_size, "Gb%d", octave); // F# = Gb
    else if (note == 8) snprintf(buffer, buffer_size, "Ab%d", octave); // G# = Ab
    else if (note == 10) snprintf(buffer, buffer_size, "Bb%d", octave); // A# = Bb
    else snprintf(buffer, buffer_size, "%s%d", note_names[note], octave);
}

} // namespace OpenChord
