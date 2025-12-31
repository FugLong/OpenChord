#pragma once

#include <cstdint>

namespace OpenChord {

/**
 * Global Octave Shift System
 * 
 * Provides a global octave offset that applies to all generated MIDI notes.
 * This allows shifting the entire keyboard range up or down, similar to
 * octave buttons on keyboards like Launchkey Mini or Arturia Keystep.
 */
class OctaveShift {
public:
    OctaveShift();
    ~OctaveShift();
    
    // Get current octave shift (-4 to +4, where 0 is no shift)
    int GetOctaveShift() const { return octave_shift_; }
    
    // Set octave shift (-4 to +4)
    void SetOctaveShift(int shift);
    
    // Apply octave shift to a MIDI note
    // Returns the shifted note (clamped to 0-127)
    uint8_t ApplyShift(uint8_t note) const;
    
    // Increment/decrement octave shift
    void IncrementOctave();
    void DecrementOctave();
    
    // Check if octave shift is active
    bool IsShifted() const { return octave_shift_ != 0; }
    
private:
    int octave_shift_;  // -4 to +4, 0 = no shift
    static constexpr int MIN_SHIFT = -4;
    static constexpr int MAX_SHIFT = 4;
};

} // namespace OpenChord

