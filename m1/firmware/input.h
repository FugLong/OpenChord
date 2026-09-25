#pragma once

#include "session.h"

#include <cstdint>

namespace ocfw {

// Snapshot a later GPIO / I2C driver will fill. This core does not read pins.
struct Input {
    bool keyswitch[oc::Session::kKeyswitchCount]{};
    bool prev = false;
    bool menu = false;
    bool next = false;
    float trackpad_x = 0.f;
    float trackpad_y = 0.f;
    bool trackpad_finger = false;
    uint8_t strip = 0;
    bool strip_finger = false;
};

// Edges in `now` become the same engine calls the plugin uses.
class SurfaceFeed {
public:
    void apply(oc::Session& session, const Input& now);

private:
    Input last_{};
    bool have_ = false;
};

} // namespace ocfw
