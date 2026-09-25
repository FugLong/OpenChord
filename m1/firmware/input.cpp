#include "input.h"

#include <cstring>

namespace ocfw {
namespace {

bool AxisChanged(float a, float b) {
    return std::memcmp(&a, &b, sizeof(float)) != 0;
}

} // namespace

void SurfaceFeed::apply(oc::Session& session, const Input& now) {
    if (!have_) {
        last_ = {};
        have_ = true;
    }

    for (int i = 0; i < oc::Session::kKeyswitchCount; ++i) {
        if (now.keyswitch[i] != last_.keyswitch[i])
            session.setKeyswitch(static_cast<uint8_t>(i), now.keyswitch[i]);
    }

    if (now.prev != last_.prev) session.setButton(oc::Button::Prev, now.prev);
    if (now.menu != last_.menu) session.setButton(oc::Button::Menu, now.menu);
    if (now.next != last_.next) session.setButton(oc::Button::Next, now.next);

    if (AxisChanged(now.trackpad_x, last_.trackpad_x)
        || AxisChanged(now.trackpad_y, last_.trackpad_y)
        || now.trackpad_finger != last_.trackpad_finger) {
        session.setTrackpad(now.trackpad_x, now.trackpad_y, now.trackpad_finger);
    }

    if (now.strip != last_.strip || now.strip_finger != last_.strip_finger)
        session.setStrip(now.strip, now.strip_finger);

    last_ = now;
}

} // namespace ocfw
