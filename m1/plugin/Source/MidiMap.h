#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace ocplug {

enum class ControlId : int {
    Keyswitch1 = 0,
    Keyswitch2,
    Keyswitch3,
    Keyswitch4,
    Keyswitch5,
    Keyswitch6,
    Keyswitch7,
    Keyswitch8,
    Prev,
    Menu,
    Next,
    TrackpadX,
    TrackpadY,
    Strip,
    Count
};

inline constexpr int kControlCount = static_cast<int>(ControlId::Count);
inline constexpr int kLegacyControlCount = 13;

inline bool IsKeyswitch(ControlId id) {
    return id >= ControlId::Keyswitch1 && id <= ControlId::Keyswitch8;
}

inline bool IsButton(ControlId id) {
    return id == ControlId::Prev || id == ControlId::Menu || id == ControlId::Next;
}

inline bool IsAxis(ControlId id) {
    return id == ControlId::TrackpadX || id == ControlId::TrackpadY || id == ControlId::Strip;
}

inline int KeyswitchIndex(ControlId id) {
    return static_cast<int>(id) - static_cast<int>(ControlId::Keyswitch1);
}

inline const char* ControlName(ControlId id) {
    switch (id) {
        case ControlId::Keyswitch1: return "1";
        case ControlId::Keyswitch2: return "2";
        case ControlId::Keyswitch3: return "3";
        case ControlId::Keyswitch4: return "4";
        case ControlId::Keyswitch5: return "5";
        case ControlId::Keyswitch6: return "6";
        case ControlId::Keyswitch7: return "7";
        case ControlId::Keyswitch8: return "8";
        case ControlId::Prev: return "Prev";
        case ControlId::Menu: return "Menu";
        case ControlId::Next: return "Next";
        case ControlId::TrackpadX: return "Track X";
        case ControlId::TrackpadY: return "Track Y";
        case ControlId::Strip: return "Strip";
        case ControlId::Count: break;
    }
    return "?";
}

struct Binding {
    enum class Kind : uint8_t { None = 0, Note, Cc };
    Kind    kind = Kind::None;
    uint8_t channel = 0; // 0 = any channel
    uint8_t number = 0;
};

class MidiMap {
public:
    MidiMap() { resetPreset(); }

    void resetPreset();
    void clear(ControlId id);
    void set(ControlId id, Binding b);

    Binding get(ControlId id) const;

    ControlId matchNote(uint8_t channel, uint8_t note) const;
    ControlId matchCc(uint8_t channel, uint8_t number) const;

    // One message, one control. A repeat assign moves it.
    void bindUnique(ControlId id, Binding b);

    void formatBinding(ControlId id, char* buf, size_t cap) const;

    static constexpr int kBlobBytes = kControlCount * 3;
    static constexpr int kLegacyBlobBytes = kLegacyControlCount * 3;
    void toBlob(uint8_t* out) const;
    void fromBlob(const uint8_t* in, int nbytes);
    void fromLegacyBlob(const uint8_t* in, int nbytes);

private:
    std::array<Binding, kControlCount> bindings_{};
};

} // namespace ocplug
