#include "session.h"

#include <cstdio>
#include <cstring>

namespace oc {
namespace {

float CcToStick(uint8_t value) {
    return (static_cast<float>(value) - 64.0f) / 64.0f;
}

bool VoicingHas(const Voicing& v, uint8_t note) {
    for (uint8_t i = 0; i < v.n; ++i) {
        if (v.notes[i] == note) return true;
    }
    return false;
}

} // namespace

Session::Session() { reset(); }

void Session::reset() {
    mode_ = PlayMode::Keys;
    for (int i = 0; i < kMaxVoices; ++i) {
        voices_[i].root = -1;
        voices_[i].chord = false;
        voices_[i].scale = false;
        voices_[i].degree_idx = 0;
        voices_[i].type = Type::None;
        voices_[i].ext = 0;
        voices_[i].v.n = 0;
        voices_[i].v.name[0] = 0;
    }
    for (int i = 0; i < 128; ++i) note_refs_[i] = 0;
    held_vel_ = 100;
    out_ch_ = 1;
    key_pc_ = 0;
    key_held_ = false;
    scale_tonic_midi_ = 48;
    type_stack_n_ = 0;
    pad_ext_ = 0;
    degree_held_ = 0;
    degree_order_n_ = 0;
    stick_x_ = 0.f;
    stick_y_ = 0.f;
    stick_x_armed_ = false;
    stick_y_armed_ = false;
    track_finger_ = false;
    for (int i = 0; i < kKeyswitchCount; ++i) {
        key_[i] = false;
        drum_on_[i] = false;
        drum_note_[i] = 0;
    }
    for (int i = 0; i < 3; ++i) button_[i] = false;
    menu_open_ = false;
    menu_index_ = 0;
    vary_ = 4;
    vary_rng_ = 1;
    harmony_ = Harmony::InKey;
    trigger_ = Trigger::Optional;
    strip_ = 0;
    strip_finger_ = false;
    last_seat_ = Seat::Home;
    out_n_ = 0;
}

void Session::syncScaleTonic() {
    int midi = (4 + static_cast<int>(octave_)) * 12 + static_cast<int>(key_pc_);
    while (midi > 127) midi -= 12;
    while (midi < 0) midi += 12;
    scale_tonic_midi_ = static_cast<int16_t>(midi);
}

void Session::setOctave(int oct) {
    if (oct < -3) oct = -3;
    if (oct > 3) oct = 3;
    if (octave_ == oct) return;
    octave_ = static_cast<int8_t>(oct);
    syncScaleTonic();
    if (mode_ != PlayMode::Scale) return;
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices_[i].root < 0 || !voices_[i].scale) continue;
        int16_t root = -1;
        Type t = Type::None;
        if (!DegreeToChord(key_pc_, voices_[i].degree_idx, scale_tonic_midi_, &root, &t))
            continue;
        voices_[i].root = root;
        voices_[i].type = t;
        if (scaleIsCurrent(voices_[i])) renderVoice(voices_[i]);
    }
}

void Session::setKeyPc(uint8_t pc) {
    key_pc_ = static_cast<uint8_t>(pc % 12);
    syncScaleTonic();
    if (mode_ == PlayMode::Scale) {
        for (int i = 0; i < kMaxVoices; ++i) {
            if (voices_[i].root < 0 || !voices_[i].scale) continue;
            int16_t root = -1;
            Type t = Type::None;
            if (!DegreeToChord(key_pc_, voices_[i].degree_idx, scale_tonic_midi_, &root, &t))
                continue;
            voices_[i].root = root;
            voices_[i].type = t;
            voices_[i].ext = 0;
            voices_[i].v.n = 0;
            if (scaleIsCurrent(voices_[i])) renderVoice(voices_[i]);
        }
    }
}

void Session::clearAllVoices() {
    for (int i = 0; i < kKeyswitchCount; ++i) {
        if (!drum_on_[i]) continue;
        midiSendOff(drum_note_[i], 10);
        drum_on_[i] = false;
    }
    drum_recent_n_ = 0;
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices_[i].root < 0) continue;
        if (voices_[i].chord) voicingOff(voices_[i].v);
        else midiSendOff(static_cast<uint8_t>(voices_[i].root));
        voices_[i].root = -1;
        voices_[i].v.n = 0;
        voices_[i].scale = false;
    }
    releaseStrip();
    arm_root_ = -1;
    arm_v_.n = 0;
    arm_v_.name[0] = 0;
    for (int n = 0; n < 128; ++n) {
        while (note_refs_[n]) midiSendOff(static_cast<uint8_t>(n));
    }
    type_stack_n_ = 0;
    pad_ext_ = 0;
    degree_held_ = 0;
    degree_order_n_ = 0;
}

void Session::setPlayMode(PlayMode mode) {
    if (mode_ == mode) return;
    clearAllVoices();
    mode_ = mode;
    last_seat_ = effectiveSeat();
    applyHeldKeyswitches();
}

Seat Session::effectiveSeat() const {
    if (!track_finger_) return Seat::Home;
    return SeatFromStick(stick_x_, stick_y_);
}

void Session::drumHit(uint8_t index, bool down) {
    if (index >= kKeyswitchCount) return;
    if (!down) {
        if (!drum_on_[index]) return;
        midiSendOff(drum_note_[index], 10);
        drum_on_[index] = false;
        uint8_t w = 0;
        for (uint8_t i = 0; i < drum_recent_n_; ++i) {
            if (drum_recent_[i] != index) drum_recent_[w++] = drum_recent_[i];
        }
        drum_recent_n_ = w;
        return;
    }
    if (drum_on_[index]) return;

    static const uint8_t kLeft[8] = {36, 38, 42, 46, 41, 47, 39, 49};
    static const uint8_t kRight[8] = {37, 44, 51, 54, 56, 50, 57, 53};
    const bool right = stick_x_ > 0.f;
    drum_right_[index] = right;
    drum_note_[index] = right ? kRight[index] : kLeft[index];
    for (uint8_t i = 0; i < drum_recent_n_; ++i) {
        if (drum_recent_[i] == index) {
            for (uint8_t j = i; j + 1 < drum_recent_n_; ++j) drum_recent_[j] = drum_recent_[j + 1];
            --drum_recent_n_;
            break;
        }
    }
    if (drum_recent_n_ < kKeyswitchCount) drum_recent_[drum_recent_n_++] = index;

    float y = stick_y_;
    if (y < -1.f) y = -1.f;
    if (y > 1.f) y = 1.f;
    int base = y >= 0.f ? static_cast<int>(100.f + y * 27.f + 0.5f)
                        : static_cast<int>(100.f + y * 99.f + 0.5f);
    if (base < 1) base = 1;
    if (base > 127) base = 127;
    // Vary always humanizes a hit. Pad height sets the center and widens the spread.
    int span = static_cast<int>(vary_);
    if (span < 2) span = 2;
    const float away = y < 0.f ? -y : y;
    span += static_cast<int>(away * static_cast<float>(span) + 0.5f);
    vary_rng_ = vary_rng_ * 1664525u + 1013904223u;
    const int offset = static_cast<int>(vary_rng_ % static_cast<uint32_t>(span * 2 + 1)) - span;
    int swung = base + offset;
    if (swung < 1) swung = 1;
    if (swung > 127) swung = 127;
    const uint8_t vel = static_cast<uint8_t>(swung);
    drum_on_[index] = true;
    midiSendOn(drum_note_[index], vel, 10);
}

void Session::applyKeyswitchMusic(uint8_t index, bool down) {
    if (menu_open_) return;
    if (mode_ == PlayMode::Drums) {
        drumHit(index, down);
        return;
    }
    if (mode_ == PlayMode::Keys) {
        if (index < 4) typePad(index, down);
        else if (index == 4) setExtBit(Ext6, down);
        else if (index == 5) setExtBit(Extm7, down);
        else if (index == 6) setExtBit(ExtM7, down);
        else if (index == 7) setExtBit(Ext9, down);
        refreshStrumArm();
        return;
    }
    if (mode_ == PlayMode::Scale) degreePad(index, down);
}

void Session::applyHeldKeyswitches() {
    if (menu_open_) return;
    if (mode_ == PlayMode::Keys) {
        type_stack_n_ = 0;
        pad_ext_ = 0;
        for (uint8_t i = 0; i < kKeyswitchCount; ++i) {
            if (key_[i]) applyKeyswitchMusic(i, true);
        }
        return;
    }
    if (mode_ == PlayMode::Scale || mode_ == PlayMode::Drums) {
        for (uint8_t i = 0; i < kKeyswitchCount; ++i) {
            if (key_[i]) applyKeyswitchMusic(i, true);
        }
    }
}

void Session::setKeyswitch(uint8_t index, bool down) {
    if (index >= kKeyswitchCount) return;
    if (key_[index] == down) return;
    key_[index] = down;
    applyKeyswitchMusic(index, down);
}

void Session::setButton(Button button, bool down) {
    const int i = static_cast<int>(button);
    if (i < 0 || i > 2) return;
    if (button_[i] == down) return;
    button_[i] = down;
    if (!down) return;

    if (button == Button::Prev || button == Button::Next) {
        const int dir = (button == Button::Next) ? 1 : -1;
        if (menu_open_) {
            menu_index_ = (menu_index_ + dir + kMenuSettings) % kMenuSettings;
            return;
        }
        int m = static_cast<int>(mode_) + dir;
        m = (m % 3 + 3) % 3;
        setPlayMode(static_cast<PlayMode>(m));
        return;
    }

    if (!menu_open_) {
        clearAllVoices();
        menu_open_ = true;
        return;
    }
    menu_open_ = false;
    applyHeldKeyswitches();
}

void Session::setTrackpad(float x, float y, bool finger) {
    if (x < -1.f) x = -1.f;
    if (x > 1.f) x = 1.f;
    if (y < -1.f) y = -1.f;
    if (y > 1.f) y = 1.f;
    stick_x_ = x;
    stick_y_ = y;
    track_finger_ = finger;
    stick_x_armed_ = true;
    stick_y_armed_ = true;
    if (menu_open_) applyMenuX(stick_x_);
    updateSeat();
}

namespace {

int MenuBucket(float x, int count) {
    if (count < 1) return 0;
    const float t = (x + 1.f) * 0.5f;
    int i = static_cast<int>(t * static_cast<float>(count));
    if (i < 0) i = 0;
    if (i >= count) i = count - 1;
    return i;
}

} // namespace

void Session::applyMenuX(float x) {
    switch (menu_index_) {
        case 0:
            setKeyPc(static_cast<uint8_t>(MenuBucket(x, 12)));
            break;
        case 1:
            setOctave(MenuBucket(x, 7) - 3);
            break;
        case 2:
            vary_ = static_cast<uint8_t>(MenuBucket(x, 8));
            break;
        case 3:
            harmony_ = static_cast<Harmony>(MenuBucket(x, 4));
            break;
        default:
            trigger_ = static_cast<Trigger>(MenuBucket(x, 2));
            break;
    }
}

void Session::releaseStrip() {
    for (uint8_t i = 0; i < strip_n_; ++i) {
        if (strip_owned_[i]) midiSendOff(strip_notes_[i]);
    }
    strip_n_ = 0;
    strip_zone_ = -1;
}

void Session::soundStripZone(const Voicing& recipe, int zone) {
    if (zone < 0 || zone >= static_cast<int>(recipe.n)) return;
    const uint8_t note = recipe.notes[zone];
    uint8_t vel = 100;
    varyVelocities(100, 1, &vel);
    for (uint8_t i = 0; i < strip_n_; ++i) {
        if (strip_notes_[i] != note) continue;
        // Entering the zone again is a new pluck. Do not wait for a finger-up.
        if (strip_owned_[i]) {
            midiSendOff(note);
            midiSendOn(note, vel);
        } else {
            pushOut(MidiEvent::Kind::NoteOn, note, vel, 0);
        }
        return;
    }
    if (strip_n_ >= 8) return;
    const bool owned = note_refs_[note] == 0;
    if (owned) midiSendOn(note, vel);
    else pushOut(MidiEvent::Kind::NoteOn, note, vel, 0);
    strip_notes_[strip_n_] = note;
    strip_owned_[strip_n_] = owned;
    ++strip_n_;
}

const Voicing* Session::stripRecipe() const {
    if (menu_open_) return nullptr;
    if (mode_ == PlayMode::Keys) {
        if (trigger_ == Trigger::Strip) return arm_root_ >= 0 ? &arm_v_ : nullptr;
        const Voicing* found = nullptr;
        for (int i = 0; i < kMaxVoices; ++i) {
            if (voices_[i].root >= 0 && voices_[i].chord && !voices_[i].scale && voices_[i].v.n)
                found = &voices_[i].v;
        }
        return found;
    }
    if (mode_ != PlayMode::Scale || degree_order_n_ == 0) return nullptr;
    const Voice* h = findScaleDegree(degree_order_[degree_order_n_ - 1]);
    if (!h || h->v.n == 0) return nullptr;
    return &h->v;
}

void Session::buildStrumArm(uint8_t pitch) {
    arm_root_ = static_cast<int16_t>(pitch);
    arm_v_.n = 0;
    arm_v_.name[0] = 0;
    const Type t = heldType();
    if (t == Type::None) {
        arm_v_.n = 1;
        arm_v_.notes[0] = pitch;
        static const char* kPc[12] = {
            "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        const int oct = static_cast<int>(pitch) / 12 - 1;
        std::snprintf(arm_v_.name, sizeof(arm_v_.name), "%s%d", kPc[pitch % 12], oct);
        return;
    }
    EngineInput in{};
    int rootMidi = static_cast<int>(pitch);
    if (t != Type::None) {
        rootMidi += static_cast<int>(octave_) * 12;
        if (rootMidi < 0) rootMidi = 0;
        if (rootMidi > 127) rootMidi = 127;
    }
    in.root_midi = static_cast<int16_t>(rootMidi);
    in.type = t;
    in.ext = pad_ext_;
    in.seat = effectiveSeat();
    in.key_pc = key_pc_;
    in.velocity = 100;
    Render(in, nullptr, &arm_v_);
}

void Session::refreshStrumArm() {
    if (mode_ != PlayMode::Keys || trigger_ != Trigger::Strip || arm_root_ < 0) return;
    const bool sounding = strip_n_ > 0;
    releaseStrip();
    buildStrumArm(static_cast<uint8_t>(arm_root_));
    if (sounding && strip_finger_ && arm_v_.n) setStrip(strip_, true);
}

void Session::setStrip(uint8_t position, bool finger) {
    strip_ = position;
    if (mode_ != PlayMode::Keys && mode_ != PlayMode::Scale) {
        strip_finger_ = finger;
        return;
    }
    if (!finger) {
        strip_finger_ = false;
        releaseStrip();
        return;
    }
    const bool was = strip_finger_;
    strip_finger_ = true;
    const Voicing* recipe = stripRecipe();
    if (!recipe || recipe->n == 0) return;
    Voicing ordered = *recipe;
    for (uint8_t i = 0; i < ordered.n; ++i) {
        for (uint8_t j = static_cast<uint8_t>(i + 1); j < ordered.n; ++j) {
            if (ordered.notes[j] < ordered.notes[i]) {
                const uint8_t t = ordered.notes[i];
                ordered.notes[i] = ordered.notes[j];
                ordered.notes[j] = t;
            }
        }
    }
    int zone = static_cast<int>(position) * ordered.n / 256;
    if (zone >= ordered.n) zone = ordered.n - 1;
    recipe = &ordered;
    if (!was || strip_zone_ < 0) {
        soundStripZone(*recipe, zone);
        strip_zone_ = zone;
        return;
    }
    const int step = zone >= strip_zone_ ? 1 : -1;
    for (int z = strip_zone_; z != zone; z += step) soundStripZone(*recipe, z + step);
    strip_zone_ = zone;
}

bool Session::strumWaiting() const {
    if (trigger_ != Trigger::Strip || strip_finger_) return false;
    if (mode_ == PlayMode::Keys) return arm_root_ >= 0;
    if (mode_ == PlayMode::Scale) return degree_held_ != 0;
    return false;
}

bool Session::keyswitchDown(uint8_t index) const {
    if (index >= kKeyswitchCount) return false;
    return key_[index];
}

bool Session::buttonDown(Button button) const {
    const int i = static_cast<int>(button);
    if (i < 0 || i > 2) return false;
    return button_[i];
}

void Session::pushOut(MidiEvent::Kind kind, uint8_t d1, uint8_t d2, uint8_t channel) {
    if (out_n_ >= kOutCap) return;
    out_q_[out_n_].kind = kind;
    out_q_[out_n_].channel = channel ? channel : out_ch_;
    out_q_[out_n_].data1 = d1;
    out_q_[out_n_].data2 = d2;
    ++out_n_;
}

void Session::midiSendOn(uint8_t note, uint8_t vel, uint8_t channel) {
    if (note > 127) return;
    if (vel == 0) vel = 1;
    if (note_refs_[note] < 255) note_refs_[note]++;
    if (note_refs_[note] == 1) pushOut(MidiEvent::Kind::NoteOn, note, vel, channel);
}

void Session::midiSendOff(uint8_t note, uint8_t channel) {
    if (note > 127) return;
    if (!note_refs_[note]) return;
    note_refs_[note]--;
    if (note_refs_[note] == 0) pushOut(MidiEvent::Kind::NoteOff, note, 0, channel);
}

void Session::voicingOff(const Voicing& v) {
    for (uint8_t i = 0; i < v.n; ++i) midiSendOff(v.notes[i]);
}

void Session::varyVelocities(uint8_t base, int count, uint8_t* out) {
    if (!out || count <= 0) return;
    if (base == 0) base = 1;
    if (count > 8) count = 8;
    if (vary_ == 0) {
        for (int i = 0; i < count; ++i) out[i] = base;
        return;
    }
    const int span = static_cast<int>(vary_);
    auto roll = [this, span]() {
        vary_rng_ = vary_rng_ * 1664525u + 1013904223u;
        return static_cast<int>(vary_rng_ % static_cast<uint32_t>(span * 2 + 1)) - span;
    };
    int raw[8];
    int sum = 0;
    for (int i = 0; i < count; ++i) {
        raw[i] = roll();
        sum += raw[i];
    }
    const int mean = count > 1 ? sum / count : 0;
    const int slide = roll();
    for (int i = 0; i < count; ++i) {
        int v = static_cast<int>(base) + raw[i] - mean + slide;
        if (v < 1) v = 1;
        if (v > 127) v = 127;
        out[i] = static_cast<uint8_t>(v);
    }
}

void Session::voicingOn(const Voicing& v, uint8_t vel, bool generated) {
    uint8_t varied[8];
    if (generated) varyVelocities(vel, v.n, varied);
    for (uint8_t i = 0; i < v.n; ++i)
        midiSendOn(v.notes[i], generated ? varied[i] : vel);
}

void Session::diffVoicing(Voicing& cur, const Voicing& next, uint8_t vel, bool generated) {
    bool same = (cur.n == next.n);
    if (same) {
        for (uint8_t i = 0; i < next.n; ++i) {
            if (cur.notes[i] != next.notes[i]) same = false;
        }
    }
    if (same) {
        cur = next;
        return;
    }
    for (uint8_t i = 0; i < cur.n; ++i) {
        if (!VoicingHas(next, cur.notes[i])) midiSendOff(cur.notes[i]);
    }
    uint8_t fresh[8];
    int freshCount = 0;
    for (uint8_t i = 0; i < next.n; ++i) {
        if (!VoicingHas(cur, next.notes[i]) && freshCount < 8)
            fresh[freshCount++] = i;
    }
    uint8_t varied[8];
    if (generated) varyVelocities(vel, freshCount, varied);
    int variedIndex = 0;
    for (uint8_t i = 0; i < next.n; ++i) {
        if (!VoicingHas(cur, next.notes[i])) {
            const uint8_t use = generated ? varied[variedIndex++] : vel;
            midiSendOn(next.notes[i], use);
        }
    }
    cur = next;
}

void Session::applyInKeyColor(const Voice& h, EngineInput& in) const {
    // One chord per wedge. interaction.md, Harmony In key. I+ uses I.
    // Later Harmony steps replace this table. Until then every step uses it.
    struct Slot {
        Type type;
        uint8_t ext;
        uint8_t inv;
        bool open;
    };
    // Seat order: Home N NE E SE S SW W NW
    static const Slot kMap[7][9] = {
        {{Type::Maj, 0, 0, false}, {Type::Maj, ExtM7, 0, false}, {Type::Maj, ExtM7, 1, false},
         {Type::Sus, 0, 0, false}, {Type::Maj, 0, 1, false}, {Type::Maj, Ext6, 0, false},
         {Type::Maj, 0, 2, false}, {Type::Sus2, 0, 0, false}, {Type::Maj, ExtM7, 2, false}},
        {{Type::Min, 0, 0, false}, {Type::Min, Extm7, 0, false}, {Type::Min, Extm7, 1, false},
         {Type::Sus, 0, 0, false}, {Type::Min, 0, 1, false}, {Type::Min, Ext6, 0, false},
         {Type::Min, 0, 2, false}, {Type::Sus2, 0, 0, false}, {Type::Min, Extm7, 2, false}},
        {{Type::Min, 0, 0, false}, {Type::Min, Extm7, 0, false}, {Type::Min, Extm7, 1, false},
         {Type::Sus, 0, 0, false}, {Type::Sus, 0, 1, false}, {Type::Min, 0, 2, false},
         {Type::Min, Extm7, 3, false}, {Type::Min, 0, 1, false}, {Type::Min, Extm7, 2, false}},
        {{Type::Maj, 0, 0, false}, {Type::Maj, ExtM7, 0, false}, {Type::Maj, ExtM7, 1, false},
         {Type::Maj, 0, 1, false}, {Type::Maj, 0, 2, false}, {Type::Maj, Ext6, 0, false},
         {Type::Sus2, 0, 2, false}, {Type::Sus2, 0, 0, false}, {Type::Maj, ExtM7, 2, false}},
        {{Type::Maj, 0, 0, false}, {Type::Maj, Extm7, 0, false}, {Type::Maj, Extm7, 1, false},
         {Type::Sus, 0, 0, false}, {Type::Maj, 0, 1, false}, {Type::Maj, Ext6, 0, false},
         {Type::Maj, 0, 2, false}, {Type::Sus2, 0, 0, false}, {Type::Maj, Extm7, 2, false}},
        {{Type::Min, 0, 0, false}, {Type::Min, Extm7, 0, false}, {Type::Min, Extm7, 1, false},
         {Type::Sus, 0, 0, false}, {Type::Sus, 0, 1, false}, {Type::Min, 0, 2, false},
         {Type::Min, Extm7, 2, false}, {Type::Sus2, 0, 0, false}, {Type::Min, Extm7, 3, false}},
        {{Type::Dim, 0, 0, false}, {Type::Dim, Extm7, 0, false}, {Type::Dim, Extm7, 1, false},
         {Type::Dim, 0, 1, false}, {Type::Dim, Extm7, 2, false}, {Type::Dim, 0, 2, false},
         {Type::Dim, 0, 0, true}, {Type::Dim, Extm7, 3, false}, {Type::Dim, Extm7, 0, true}},
    };
    const uint8_t degree = h.degree_idx == 7 ? 0 : h.degree_idx;
    if (degree > 6) return;
    const int seat = static_cast<int>(in.seat);
    if (seat < 0 || seat > 8) return;
    const Slot slot = kMap[degree][seat];
    in.type = slot.type;
    in.ext = slot.ext;
    in.color = true;
    in.color_inv = slot.inv;
    in.color_open = slot.open;
    in.seat = Seat::Home;
}

void Session::renderVoice(Voice& h) {
    if (!h.chord || h.root < 0) return;
    EngineInput in{};
    int rootMidi = h.root;
    if (!h.scale) {
        rootMidi += static_cast<int>(octave_) * 12;
        if (rootMidi < 0) rootMidi = 0;
        if (rootMidi > 127) rootMidi = 127;
    }
    in.root_midi = static_cast<int16_t>(rootMidi);
    in.type = h.type;
    in.ext = h.ext;
    in.seat = effectiveSeat();
    if (h.scale && !scaleIsCurrent(h)) return;
    if (h.scale) applyInKeyColor(h, in);
    in.key_pc = key_pc_;
    const bool generated = h.scale;
    const uint8_t base = generated ? 100 : held_vel_;
    in.velocity = base;
    Voicing next{};
    // Scale Color is the recipe for that direction. Skip voice-leading so an
    // inversion is not pulled back onto the triad you were already holding.
    Render(in, h.scale ? nullptr : (h.v.n ? &h.v : nullptr), &next);
    if (h.scale && trigger_ == Trigger::Strip) {
        h.v = next;
        return;
    }
    if (h.v.n == 0) {
        h.v = next;
        voicingOn(h.v, base, generated);
        return;
    }
    diffVoicing(h.v, next, base, generated);
}

Session::Voice* Session::findVoice(int16_t root) {
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices_[i].root == root) return &voices_[i];
    }
    return nullptr;
}

Session::Voice* Session::findScaleDegree(uint8_t degree_idx) {
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices_[i].root >= 0 && voices_[i].scale && voices_[i].degree_idx == degree_idx)
            return &voices_[i];
    }
    return nullptr;
}

const Session::Voice* Session::findScaleDegree(uint8_t degree_idx) const {
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices_[i].root >= 0 && voices_[i].scale && voices_[i].degree_idx == degree_idx)
            return &voices_[i];
    }
    return nullptr;
}

Session::Voice* Session::allocVoice() {
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices_[i].root < 0) return &voices_[i];
    }
    return nullptr;
}

Type Session::heldType() const {
    if (type_stack_n_ == 0) return Type::None;
    return static_cast<Type>(type_stack_[type_stack_n_ - 1]);
}

uint8_t Session::typeHeldMask() const {
    uint8_t mask = 0;
    for (uint8_t i = 0; i < type_stack_n_; ++i) {
        const uint8_t t = type_stack_[i];
        if (t >= 1 && t <= 4)
            mask = static_cast<uint8_t>(mask | (1u << (t - 1)));
    }
    return mask;
}

uint8_t Session::degreeHeldMask() const { return degree_held_; }

bool Session::typeStillHeld(Type t) const {
    uint8_t want = static_cast<uint8_t>(t);
    for (uint8_t i = 0; i < type_stack_n_; ++i) {
        if (type_stack_[i] == want) return true;
    }
    return false;
}

void Session::typePad(uint8_t idx, bool on) {
    if (mode_ != PlayMode::Keys) return;
    if (idx > 3) return;
    uint8_t t = static_cast<uint8_t>(idx + 1);
    if (on) {
        for (uint8_t i = 0; i < type_stack_n_; ++i) {
            if (type_stack_[i] == t) {
                for (uint8_t j = i; j + 1 < type_stack_n_; ++j)
                    type_stack_[j] = type_stack_[j + 1];
                type_stack_n_--;
                break;
            }
        }
        if (type_stack_n_ < 4) type_stack_[type_stack_n_++] = t;
        return;
    }
    uint8_t w = 0;
    for (uint8_t i = 0; i < type_stack_n_; ++i) {
        if (type_stack_[i] != t) type_stack_[w++] = type_stack_[i];
    }
    type_stack_n_ = w;
}

void Session::setExtBit(uint8_t bit, bool on) {
    if (mode_ != PlayMode::Keys) return;
    if (on) pad_ext_ = static_cast<uint8_t>(pad_ext_ | bit);
    else pad_ext_ = static_cast<uint8_t>(pad_ext_ & ~bit);
    refreshHeldChordExts();
}

bool Session::scaleIsCurrent(const Voice& h) const {
    if (!h.scale) return true;
    if (degree_order_n_ == 0) return false;
    return h.degree_idx == degree_order_[degree_order_n_ - 1];
}

void Session::quietScale(Voice& h) {
    if (trigger_ != Trigger::Strip) voicingOff(h.v);
    h.v.n = 0;
    h.v.name[0] = 0;
}

void Session::followScale(uint8_t idx, bool on) {
    if (on) {
        for (uint8_t i = 0; i < degree_order_n_; ++i) {
            if (degree_order_[i] != idx) continue;
            for (uint8_t j = i; j + 1 < degree_order_n_; ++j)
                degree_order_[j] = degree_order_[j + 1];
            degree_order_n_--;
            break;
        }
        if (degree_order_n_ < 8) degree_order_[degree_order_n_++] = idx;
        return;
    }
    uint8_t w = 0;
    for (uint8_t i = 0; i < degree_order_n_; ++i) {
        if (degree_order_[i] != idx) degree_order_[w++] = degree_order_[i];
    }
    degree_order_n_ = w;
}

void Session::degreePad(uint8_t idx, bool on) {
    if (mode_ != PlayMode::Scale) return;
    if (idx > 7) return;

    const uint8_t bit = static_cast<uint8_t>(1u << idx);
    if (on) {
        if (degree_held_ & bit) return;
        const bool finger = strip_finger_;
        if (finger) releaseStrip();
        if (degree_order_n_ > 0) {
            if (Voice* prev = findScaleDegree(degree_order_[degree_order_n_ - 1]))
                quietScale(*prev);
        }
        degree_held_ = static_cast<uint8_t>(degree_held_ | bit);
        followScale(idx, true);
        if (!findScaleDegree(idx)) {
            int16_t root = -1;
            Type t = Type::None;
            if (!DegreeToChord(key_pc_, idx, scale_tonic_midi_, &root, &t)) return;
            Voice* h = allocVoice();
            if (!h) return;
            h->root = root;
            h->chord = true;
            h->scale = true;
            h->degree_idx = idx;
            h->type = t;
            h->ext = 0;
            h->v.n = 0;
            h->v.name[0] = 0;
        }
        if (Voice* h = findScaleDegree(idx)) renderVoice(*h);
        if (finger) setStrip(strip_, true);
        return;
    }

    const bool wasCurrent = degree_order_n_ > 0 && degree_order_[degree_order_n_ - 1] == idx;
    degree_held_ = static_cast<uint8_t>(degree_held_ & ~bit);
    followScale(idx, false);
    Voice* h = findScaleDegree(idx);
    if (h) {
        if (trigger_ != Trigger::Strip) voicingOff(h->v);
        h->root = -1;
        h->v.n = 0;
        h->scale = false;
    }
    if (!wasCurrent) return;
    const bool finger = strip_finger_;
    if (finger) releaseStrip();
    if (degree_order_n_ > 0) {
        if (Voice* next = findScaleDegree(degree_order_[degree_order_n_ - 1]))
            renderVoice(*next);
    }
    if (finger && degree_order_n_ > 0) setStrip(strip_, true);
    else if (finger) releaseStrip();
}

void Session::refreshHeldChordExts() {
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices_[i].root < 0 || !voices_[i].chord || voices_[i].scale) continue;
        if (!typeStillHeld(voices_[i].type)) continue;
        voices_[i].ext = pad_ext_;
        renderVoice(voices_[i]);
    }
}

void Session::stickCc(bool* armed, float* axis, uint8_t value) {
    if (!*armed) {
        if (value < 48 || value > 80) return;
        *armed = true;
    }
    *axis = CcToStick(value);
    track_finger_ = true;
}

void Session::stickCcX(uint8_t value) { stickCc(&stick_x_armed_, &stick_x_, value); }
void Session::stickCcY(uint8_t value) { stickCc(&stick_y_armed_, &stick_y_, value); }

void Session::setStickX(float x) {
    stick_x_armed_ = true;
    if (x < -1.f) x = -1.f;
    if (x > 1.f) x = 1.f;
    stick_x_ = x;
    track_finger_ = true;
}

void Session::setStickY(float y) {
    stick_y_armed_ = true;
    if (y < -1.f) y = -1.f;
    if (y > 1.f) y = 1.f;
    stick_y_ = y;
    track_finger_ = true;
}

void Session::setKeyHeld(bool held) { key_held_ = held; }

void Session::updateSeat() {
    Seat seat = effectiveSeat();
    if (seat == last_seat_) return;
    last_seat_ = seat;
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices_[i].root >= 0 && voices_[i].chord && scaleIsCurrent(voices_[i]))
            renderVoice(voices_[i]);
    }
    refreshStrumArm();
}

void Session::noteOn(uint8_t channel, uint8_t pitch, uint8_t velocity) {
    if (velocity == 0) {
        noteOff(channel, pitch);
        return;
    }
    out_ch_ = channel ? channel : 1;
    if (menu_open_) {
        if (menu_index_ == 0)
            setKeyPc(static_cast<uint8_t>(pitch % 12));
        return;
    }
    if (key_held_) {
        setKeyPc(static_cast<uint8_t>(pitch % 12));
        return;
    }

    // Scale and Drums: the keyboard is optional. Unassigned notes pass thru.
    if (mode_ == PlayMode::Scale || mode_ == PlayMode::Drums) {
        int16_t root = static_cast<int16_t>(pitch);
        if (findVoice(root)) return;
        Voice* h = allocVoice();
        if (!h) return;
        held_vel_ = velocity ? velocity : held_vel_;
        h->root = root;
        h->chord = false;
        h->scale = false;
        h->type = Type::None;
        h->ext = 0;
        h->v.n = 0;
        midiSendOn(pitch, velocity);
        return;
    }

    if (trigger_ == Trigger::Strip) {
        releaseStrip();
        buildStrumArm(pitch);
        if (strip_finger_ && arm_v_.n) {
            int zone = static_cast<int>(strip_) * arm_v_.n / 256;
            if (zone >= arm_v_.n) zone = arm_v_.n - 1;
            soundStripZone(arm_v_, zone);
            strip_zone_ = zone;
        }
        return;
    }

    int16_t root = static_cast<int16_t>(pitch);
    if (findVoice(root)) return;
    Voice* h = allocVoice();
    if (!h) return;
    held_vel_ = velocity ? velocity : held_vel_;
    Type t = heldType();
    h->root = root;
    h->v.n = 0;
    h->v.name[0] = 0;
    h->scale = false;
    if (t != Type::None) {
        h->chord = true;
        h->type = t;
        h->ext = pad_ext_;
        renderVoice(*h);
    } else {
        h->chord = false;
        h->type = Type::None;
        h->ext = 0;
        midiSendOn(pitch, velocity);
    }
}

void Session::noteOff(uint8_t /*channel*/, uint8_t pitch) {
    int16_t root = static_cast<int16_t>(pitch);
    Voice* h = findVoice(root);
    if (!h || h->scale) return;
    if (h->chord) voicingOff(h->v);
    else midiSendOff(pitch);
    h->root = -1;
    h->v.n = 0;
}

void Session::panic() {
    clearAllVoices();
    pushOut(MidiEvent::Kind::Cc, 123, 0, 0);
}

int Session::drain(MidiEvent* out, int max) {
    if (!out || max <= 0) return 0;
    int n = out_n_ < max ? out_n_ : max;
    for (int i = 0; i < n; ++i) out[i] = out_q_[i];
    if (n < out_n_) {
        int remain = out_n_ - n;
        for (int i = 0; i < remain; ++i) out_q_[i] = out_q_[i + n];
        out_n_ = remain;
    } else {
        out_n_ = 0;
    }
    return n;
}

const char* drumName(int index, bool right) {
    static const char* kLeft[8] = {
        "Kick", "Snare", "Closed hat", "Open hat", "Low tom", "Mid tom", "Clap", "Crash"};
    static const char* kRight[8] = {
        "Rim", "Pedal hat", "Ride", "Tambourine", "Cowbell", "High tom", "Crash 2", "Ride bell"};
    if (index < 0 || index > 7) return "?";
    return right ? kRight[index] : kLeft[index];
}

namespace {

void CopyLine(char* dst, size_t cap, const char* src) {
    if (!dst || cap == 0) return;
    if (!src) src = "";
    std::strncpy(dst, src, cap - 1);
    dst[cap - 1] = 0;
}

const char* PcName(uint8_t pc) {
    static const char* kPc[12] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    return kPc[pc % 12];
}

} // namespace

void Session::fillScreen(char* top, size_t topCap, char* left, size_t leftCap, char* mid,
                         size_t midCap, char* right, size_t rightCap, int* zoneCount, int* zone) const {
    if (zoneCount) *zoneCount = 0;
    if (zone) *zone = -1;
    CopyLine(left, leftCap, "");
    CopyLine(mid, midCap, "");
    CopyLine(right, rightCap, "");
    if (menu_open_) {
        const char* setting = "Key";
        char value[16];
        CopyLine(value, sizeof(value), PcName(key_pc_));
        if (menu_index_ == 1) {
            setting = "Octave";
            if (octave_ > 0) std::snprintf(value, sizeof(value), "+%d", static_cast<int>(octave_));
            else std::snprintf(value, sizeof(value), "%d", static_cast<int>(octave_));
        } else if (menu_index_ == 2) {
            setting = "Vary";
            if (vary_ == 0) CopyLine(value, sizeof(value), "off");
            else std::snprintf(value, sizeof(value), "%d", static_cast<int>(vary_));
        } else if (menu_index_ == 3) {
            setting = "Harmony";
            static const char* kH[] = {"In key", "Tensions", "Borrowed", "Free"};
            CopyLine(value, sizeof(value), kH[static_cast<int>(harmony_) & 3]);
        } else if (menu_index_ == 4) {
            setting = "Strum";
            CopyLine(value, sizeof(value), trigger_ == Trigger::Strip ? "Only" : "Optional");
        }
        CopyLine(top, topCap, setting);
        CopyLine(left, leftCap, "<");
        CopyLine(mid, midCap, value);
        CopyLine(right, rightCap, ">");
        return;
    }

    if (mode_ == PlayMode::Drums) {
        if (drum_recent_n_ > 0) {
            const uint8_t idx = drum_recent_[drum_recent_n_ - 1];
            CopyLine(top, topCap, drumName(idx, drum_right_[idx]));
            CopyLine(right, rightCap, drum_right_[idx] ? "Right" : "Left");
        } else {
            CopyLine(top, topCap, stick_x_ > 0.f ? "Right" : "Left");
        }
        CopyLine(mid, midCap, "Drums");
        return;
    }

    char name[24];
    chordName(name, sizeof(name));
    if (!name[0] && mode_ == PlayMode::Keys) {
        uint8_t held[16];
        int heldN = 0;
        for (int i = 0; i < kMaxVoices && heldN < 16; ++i) {
            if (voices_[i].root < 0 || voices_[i].chord || voices_[i].scale) continue;
            held[heldN++] = static_cast<uint8_t>(voices_[i].root);
        }
        if (heldN >= 2) NameHeardChord(held, heldN, name, sizeof(name));
        if (!name[0] && heldN >= 1) {
            int note = held[0];
            for (int i = 1; i < heldN; ++i)
                if (held[i] < note) note = held[i];
            std::snprintf(name, sizeof(name), "%s%d", PcName(static_cast<uint8_t>(note % 12)),
                          note / 12 - 1);
        }
    }
    CopyLine(top, topCap, name[0] ? name : "---");

    char key[12];
    if (octave_ > 0)
        std::snprintf(key, sizeof(key), "%s+%d", PcName(key_pc_), static_cast<int>(octave_));
    else if (octave_ < 0)
        std::snprintf(key, sizeof(key), "%s%d", PcName(key_pc_), static_cast<int>(octave_));
    else
        CopyLine(key, sizeof(key), PcName(key_pc_));
    CopyLine(left, leftCap, key);
    CopyLine(mid, midCap, mode_ == PlayMode::Scale ? "Scale" : "Keys");
    if (strumWaiting()) CopyLine(right, rightCap, "arm");

    if (!strip_finger_) return;
    const Voicing* recipe = stripRecipe();
    if (!recipe || recipe->n == 0) return;
    if (zoneCount) *zoneCount = recipe->n;
    int z = strip_zone_;
    if (z < 0) {
        z = static_cast<int>(strip_) * recipe->n / 256;
        if (z >= recipe->n) z = recipe->n - 1;
    }
    if (zone) *zone = z;
}

void Session::chordName(char* buf, size_t cap) const {
    if (!buf || cap == 0) return;
    buf[0] = 0;
    if (trigger_ == Trigger::Strip && arm_root_ >= 0 && arm_v_.name[0]) {
        std::strncpy(buf, arm_v_.name, cap - 1);
        buf[cap - 1] = 0;
        return;
    }
    if (mode_ == PlayMode::Scale) {
        if (degree_order_n_ == 0) return;
        const Voice* h = findScaleDegree(degree_order_[degree_order_n_ - 1]);
        if (!h || !h->v.name[0]) return;
        std::strncpy(buf, h->v.name, cap - 1);
        buf[cap - 1] = 0;
        return;
    }
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices_[i].root >= 0 && voices_[i].chord && voices_[i].v.name[0]) {
            std::strncpy(buf, voices_[i].v.name, cap - 1);
            buf[cap - 1] = 0;
            return;
        }
    }
}

} // namespace oc
