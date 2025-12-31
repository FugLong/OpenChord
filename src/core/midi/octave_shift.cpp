#include "octave_shift.h"
#include <algorithm>

namespace OpenChord {

OctaveShift::OctaveShift()
    : octave_shift_(0)
{
}

OctaveShift::~OctaveShift() {
}

void OctaveShift::SetOctaveShift(int shift) {
    octave_shift_ = std::max(MIN_SHIFT, std::min(MAX_SHIFT, shift));
}

void OctaveShift::IncrementOctave() {
    if (octave_shift_ < MAX_SHIFT) {
        octave_shift_++;
    }
}

void OctaveShift::DecrementOctave() {
    if (octave_shift_ > MIN_SHIFT) {
        octave_shift_--;
    }
}

uint8_t OctaveShift::ApplyShift(uint8_t note) const {
    if (octave_shift_ == 0) {
        return note;
    }
    
    int shifted_note = static_cast<int>(note) + (octave_shift_ * 12);
    
    // Clamp to valid MIDI range (0-127)
    if (shifted_note < 0) {
        return 0;
    } else if (shifted_note > 127) {
        return 127;
    }
    
    return static_cast<uint8_t>(shifted_note);
}

} // namespace OpenChord

