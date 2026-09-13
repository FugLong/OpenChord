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

} // namespace

void MidiMap::resetToLaunchkey() {
    for (auto& b : bindings_) b = {};

    auto cc = [](uint8_t ch, uint8_t n) {
        Binding b;
        b.kind = Binding::Kind::Cc;
        b.channel = ch;
        b.number = n;
        return b;
    };

    // Launchkey Mini MK4: pads on ch 10, stick knobs typically ch 1.
    // channel 0 = any — matches proto (no channel filter on CCs).
    bindings_[static_cast<int>(ControlId::Dim)] = cc(0, 36);
    bindings_[static_cast<int>(ControlId::Min)] = cc(0, 37);
    bindings_[static_cast<int>(ControlId::Maj)] = cc(0, 38);
    bindings_[static_cast<int>(ControlId::Sus)] = cc(0, 39);
    bindings_[static_cast<int>(ControlId::Ext6)] = cc(0, 40);
    bindings_[static_cast<int>(ControlId::Extm7)] = cc(0, 41);
    bindings_[static_cast<int>(ControlId::ExtM7)] = cc(0, 42);
    bindings_[static_cast<int>(ControlId::Ext9)] = cc(0, 43);
    bindings_[static_cast<int>(ControlId::StickX)] = cc(0, 47);
    bindings_[static_cast<int>(ControlId::StickY)] = cc(0, 48);
    // Key / Panic / Shift unbound by default
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
    for (int i = 0; i < kControlCount; ++i) {
        Binding b;
        b.kind = static_cast<Binding::Kind>(in[i * 3 + 0]);
        if (b.kind > Binding::Kind::Cc) b.kind = Binding::Kind::None;
        b.channel = in[i * 3 + 1];
        b.number = in[i * 3 + 2];
        bindings_[static_cast<size_t>(i)] = b;
    }
}

} // namespace ocplug
