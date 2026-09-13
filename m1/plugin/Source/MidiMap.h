#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace ocplug {

enum class ControlId : int {
    Dim = 0,
    Min,
    Maj,
    Sus,
    Ext6,
    Extm7,
    ExtM7,
    Ext9,
    StickX,
    StickY,
    Key,
    Panic,
    Shift,
    Count
};

inline constexpr int kControlCount = static_cast<int>(ControlId::Count);

struct Binding {
    enum class Kind : uint8_t { None = 0, Note, Cc };
    Kind    kind = Kind::None;
    uint8_t channel = 0; // 0 = any channel
    uint8_t number = 0;
};

inline const char* ControlName(ControlId id) {
    switch (id) {
        case ControlId::Dim: return "Dim";
        case ControlId::Min: return "Min";
        case ControlId::Maj: return "Maj";
        case ControlId::Sus: return "Sus";
        case ControlId::Ext6: return "6";
        case ControlId::Extm7: return "m7";
        case ControlId::ExtM7: return "M7";
        case ControlId::Ext9: return "9";
        case ControlId::StickX: return "Stick X";
        case ControlId::StickY: return "Stick Y";
        case ControlId::Key: return "Key";
        case ControlId::Panic: return "Panic";
        case ControlId::Shift: return "Shift";
        default: return "?";
    }
}

class MidiMap {
public:
    MidiMap() { resetToLaunchkey(); }

    void resetToLaunchkey();
    void clear(ControlId id);
    void set(ControlId id, Binding b);

    Binding get(ControlId id) const;

    // Returns control if this message matches a binding, else Count.
    ControlId matchNote(uint8_t channel, uint8_t note) const;
    ControlId matchCc(uint8_t channel, uint8_t number) const;

    // Learn: move any existing binding of this MIDI message to `id`.
    void bindUnique(ControlId id, Binding b);

    void formatBinding(ControlId id, char* buf, size_t cap) const;

    // Serialize / deserialize (fixed blob for plugin state).
    static constexpr int kBlobBytes = kControlCount * 3; // kind, ch, number each
    void toBlob(uint8_t* out) const;
    void fromBlob(const uint8_t* in, int nbytes);

private:
    std::array<Binding, kControlCount> bindings_{};
};

} // namespace ocplug
