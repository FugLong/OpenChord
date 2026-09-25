#pragma once

#include <cstddef>
#include <cstdint>

namespace oc {

// Chord quality (triad family). Not a play-mode name.
enum class Type : uint8_t { None = 0, Dim, Min, Maj, Sus, Sus2 };

enum Ext : uint8_t {
    Ext6  = 1 << 0,
    ExtM7 = 1 << 1,
    Extm7 = 1 << 2,
    Ext9  = 1 << 3,
};

enum class Seat : uint8_t {
    Home = 0,
    N, NE, E, SE, S, SW, W, NW
};

// Product modes. Screen words: "Keys", "Scale", "Drums".
// Saved state uses 0 and 1 for Keys and Scale.
enum class PlayMode : uint8_t {
    Keys  = 0, // incoming MIDI note = root; keyswitches = Dim/Min/Maj/Sus + extras
    Scale = 1, // keyswitches = I–vii + I one octave up
    Drums = 2, // keyswitches = drum hits; kit comes later
};

struct EngineInput {
    int16_t  root_midi;   // 0-127, or -1 if no root
    Type     type;
    uint8_t  ext;         // Ext bits
    Seat     seat;
    uint8_t  key_pc;      // 0-11, major for v0
    uint8_t  velocity;    // 1-127
    bool     color = false;      // Scale Color: inversion and spread come from the fields below
    uint8_t  color_inv = 0;      // 0 root, 1 = 3rd in bass, 2 = 5th, 3 = 7th
    bool     color_open = false; // same pitch classes, middle voices up an octave
};

struct Voicing {
    uint8_t n;
    uint8_t notes[8];
    char    name[20];
};

Seat SeatFromStick(float x, float y, float dead = 0.42f);

void Render(const EngineInput& in, const Voicing* prev, Voicing* out);

// Scale-mode helper: degree_idx 0..6 = I..vii, 7 = I one octave up.
// HOME is a diatonic triad (no 7th). Writes root MIDI + quality for Render().
// tonic_midi is the I root in the desired octave (e.g. 48 + key_pc).
bool DegreeToChord(uint8_t key_pc, uint8_t degree_idx, int16_t tonic_midi,
                   int16_t* root_midi_out, Type* type_out);

// Name a chord from the MIDI notes actually held. False when there are fewer
// than two pitch classes. `out` is a short symbol such as `Cm7` or `C/E`.
bool NameHeardChord(const uint8_t* notes, int n, char* out, size_t cap);

} // namespace oc
