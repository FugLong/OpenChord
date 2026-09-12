#pragma once

#include <cstdint>

namespace oc {

enum class Type : uint8_t { None = 0, Dim, Min, Maj, Sus };

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

struct EngineInput {
    int16_t  root_midi;   // 0-127, or -1 if no root
    Type     type;
    uint8_t  ext;         // Ext bits
    Seat     seat;
    uint8_t  key_pc;      // 0-11, major for v0
    uint8_t  velocity;    // 1-127
};

struct Voicing {
    uint8_t n;
    uint8_t notes[8];
    char    name[20];
};

Seat SeatFromStick(float x, float y, float dead = 0.42f);

void Render(const EngineInput& in, const Voicing* prev, Voicing* out);

} // namespace oc
