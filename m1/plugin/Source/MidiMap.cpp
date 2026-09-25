#include "MidiMap.h"

#include <cstdio>

namespace ocplug {
namespace {

bool ChannelMatch(uint8_t bound, uint8_t msg) {
    return bound == 0 || bound == msg;
}

const char* NoteName(uint8_t note) {
    static const char* kNames[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return kNames[note % 12];
}

Binding ReadBinding(const uint8_t* in) {
    Binding b;
    b.kind = static_cast<Binding::Kind>(in[0]);
    if (b.kind > Binding::Kind::Cc) b.kind = Binding::Kind::None;
    b.channel = in[1];
    b.number = in[2];
    return b;
}

} // namespace

void MidiMap::resetPreset() {
    for (auto& b : bindings_) b = {};

    // Eight controls as CC 36..43, trackpad axes as CC 47 and 48.
    // Channel 0 matches any channel.
    auto cc = [](uint8_t n) {
        Binding b;
        b.kind = Binding::Kind::Cc;
        b.channel = 0;
        b.number = n;
        return b;
    };

    for (int i = 0; i < 8; ++i)
        bindings_[static_cast<size_t>(i)] = cc(static_cast<uint8_t>(36 + i));
    bindings_[static_cast<int>(ControlId::TrackpadX)] = cc(47);
    bindings_[static_cast<int>(ControlId::TrackpadY)] = cc(48);
}

void MidiMap::clear(ControlId id) {
    const int i = static_cast<int>(id);
    if (i < 0 || i >= kControlCount) return;
    bindings_[static_cast<size_t>(i)] = {};
}

void MidiMap::set(ControlId id, Binding b) {
    const int i = static_cast<int>(id);
    if (i < 0 || i >= kControlCount) return;
    bindings_[static_cast<size_t>(i)] = b;
}

Binding MidiMap::get(ControlId id) const {
    const int i = static_cast<int>(id);
    if (i < 0 || i >= kControlCount) return {};
    return bindings_[static_cast<size_t>(i)];
}

ControlId MidiMap::matchNote(uint8_t channel, uint8_t note) const {
    for (int i = 0; i < kControlCount; ++i) {
        const auto& b = bindings_[static_cast<size_t>(i)];
        if (b.kind == Binding::Kind::Note && b.number == note && ChannelMatch(b.channel, channel))
            return static_cast<ControlId>(i);
    }
    return ControlId::Count;
}

ControlId MidiMap::matchCc(uint8_t channel, uint8_t number) const {
    for (int i = 0; i < kControlCount; ++i) {
        const auto& b = bindings_[static_cast<size_t>(i)];
        if (b.kind == Binding::Kind::Cc && b.number == number && ChannelMatch(b.channel, channel))
            return static_cast<ControlId>(i);
    }
    return ControlId::Count;
}

void MidiMap::bindUnique(ControlId id, Binding b) {
    for (int i = 0; i < kControlCount; ++i) {
        if (static_cast<ControlId>(i) == id) continue;
        const auto& o = bindings_[static_cast<size_t>(i)];
        if (o.kind == b.kind && o.number == b.number && o.channel == b.channel)
            bindings_[static_cast<size_t>(i)] = {};
    }
    set(id, b);
}

void MidiMap::formatBinding(ControlId id, char* buf, size_t cap) const {
    if (!buf || cap == 0) return;
    Binding b = get(id);
    if (b.kind == Binding::Kind::None) {
        std::snprintf(buf, cap, "%s", ControlName(id));
        return;
    }
    if (b.kind == Binding::Kind::Cc) {
        std::snprintf(buf, cap, "%s\nCC %u", ControlName(id), static_cast<unsigned>(b.number));
        return;
    }
    int oct = static_cast<int>(b.number / 12) - 2;
    std::snprintf(buf, cap, "%s\n%s%d", ControlName(id), NoteName(b.number), oct);
}

void MidiMap::toBlob(uint8_t* out) const {
    if (!out) return;
    for (int i = 0; i < kControlCount; ++i) {
        const auto& b = bindings_[static_cast<size_t>(i)];
        out[i * 3 + 0] = static_cast<uint8_t>(b.kind);
        out[i * 3 + 1] = b.channel;
        out[i * 3 + 2] = b.number;
    }
}

void MidiMap::fromBlob(const uint8_t* in, int nbytes) {
    if (!in || nbytes < kBlobBytes) return;
    for (int i = 0; i < kControlCount; ++i)
        bindings_[static_cast<size_t>(i)] = ReadBinding(in + i * 3);
}

void MidiMap::fromLegacyBlob(const uint8_t* in, int nbytes) {
    if (!in || nbytes < kLegacyBlobBytes) return;
    for (auto& b : bindings_) b = {};
    for (int i = 0; i < 8; ++i)
        bindings_[static_cast<size_t>(i)] = ReadBinding(in + i * 3);
    bindings_[static_cast<int>(ControlId::TrackpadX)] = ReadBinding(in + 8 * 3);
    bindings_[static_cast<int>(ControlId::TrackpadY)] = ReadBinding(in + 9 * 3);
}

} // namespace ocplug
