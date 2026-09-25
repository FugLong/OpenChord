#include "switches.h"

#include <Arduino.h>

#include "board.h"

namespace ocfw {
namespace {

constexpr uint32_t kDebounceMs = 12;

struct Edge {
    bool stable = false;
    bool raw = false;
    uint32_t changed_ms = 0;
};

int SpatialGpio(int spatial) {
    const int sw = ocboard::kKeyswitchSpatialSw[spatial];
    return ocboard::kKeyswitchGpio[sw - 1];
}

bool Settled(Edge& edge, bool raw, uint32_t now) {
    if (raw != edge.raw) {
        edge.raw = raw;
        edge.changed_ms = now;
    }
    if (now - edge.changed_ms >= kDebounceMs) edge.stable = edge.raw;
    return edge.stable;
}

Edge keys_[ocboard::kKeyswitchCount];
Edge prev_{};
Edge menu_{};
Edge next_{};

} // namespace

void ReadSwitches(Input& in) {
    const uint32_t now = millis();
    for (int i = 0; i < ocboard::kKeyswitchCount; ++i) {
        const bool pressed = digitalRead(SpatialGpio(i)) == LOW;
        in.keyswitch[i] = Settled(keys_[i], pressed, now);
    }
    in.prev = Settled(prev_, digitalRead(ocboard::kPrevGpio) == LOW, now);
    in.menu = Settled(menu_, digitalRead(ocboard::kMenuGpio) == LOW, now);
    in.next = Settled(next_, digitalRead(ocboard::kNextGpio) == LOW, now);
}

void InitSwitches() {
    for (int i = 0; i < ocboard::kKeyswitchCount; ++i)
        pinMode(SpatialGpio(i), INPUT_PULLUP);
    pinMode(ocboard::kPrevGpio, INPUT_PULLUP);
    pinMode(ocboard::kMenuGpio, INPUT_PULLUP);
    pinMode(ocboard::kNextGpio, INPUT_PULLUP);
}

} // namespace ocfw
