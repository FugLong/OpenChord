#include "session.h"

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
    mode_ = PlayMode::Pro;
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
    stick_x_ = 0.f;
    stick_y_ = 0.f;
    stick_x_armed_ = false;
    stick_y_armed_ = false;
    last_seat_ = Seat::Home;
    out_n_ = 0;
}

void Session::syncScaleTonic() {
    const int oct = scale_tonic_midi_ / 12;
    scale_tonic_midi_ = static_cast<int16_t>(oct * 12 + key_pc_);
}

void Session::setKeyPc(uint8_t pc) {
    key_pc_ = static_cast<uint8_t>(pc % 12);
    syncScaleTonic();
    if (mode_ == PlayMode::Smart) {
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
            renderVoice(voices_[i]);
        }
    }
}

void Session::clearAllVoices() {
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices_[i].root < 0) continue;
        if (voices_[i].chord) voicingOff(voices_[i].v);
        else midiSendOff(static_cast<uint8_t>(voices_[i].root));
        voices_[i].root = -1;
        voices_[i].v.n = 0;
        voices_[i].scale = false;
    }
    for (int n = 0; n < 128; ++n) {
        while (note_refs_[n]) midiSendOff(static_cast<uint8_t>(n));
    }
    type_stack_n_ = 0;
    pad_ext_ = 0;
    degree_held_ = 0;
}

void Session::setPlayMode(PlayMode mode) {
    if (mode_ == mode) return;
    clearAllVoices();
    mode_ = mode;
    last_seat_ = SeatFromStick(stick_x_, stick_y_);
}

void Session::pushOut(MidiEvent::Kind kind, uint8_t d1, uint8_t d2) {
    if (out_n_ >= kOutCap) return;
    out_q_[out_n_].kind = kind;
    out_q_[out_n_].channel = out_ch_;
    out_q_[out_n_].data1 = d1;
    out_q_[out_n_].data2 = d2;
    ++out_n_;
}

void Session::midiSendOn(uint8_t note, uint8_t vel) {
    if (note > 127) return;
    if (vel == 0) vel = 1;
    if (note_refs_[note] < 255) note_refs_[note]++;
    if (note_refs_[note] == 1) pushOut(MidiEvent::Kind::NoteOn, note, vel);
}

void Session::midiSendOff(uint8_t note) {
    if (note > 127) return;
    if (!note_refs_[note]) return;
    note_refs_[note]--;
    if (note_refs_[note] == 0) pushOut(MidiEvent::Kind::NoteOff, note, 0);
}

void Session::voicingOff(const Voicing& v) {
    for (uint8_t i = 0; i < v.n; ++i) midiSendOff(v.notes[i]);
}

void Session::voicingOn(const Voicing& v, uint8_t vel) {
    for (uint8_t i = 0; i < v.n; ++i) midiSendOn(v.notes[i], vel);
}

void Session::diffVoicing(Voicing& cur, const Voicing& next, uint8_t vel) {
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
    for (uint8_t i = 0; i < next.n; ++i) {
        if (!VoicingHas(cur, next.notes[i])) midiSendOn(next.notes[i], vel);
    }
    cur = next;
}

void Session::renderVoice(Voice& h) {
    if (!h.chord || h.root < 0) return;
    EngineInput in{};
    in.root_midi = h.root;
    in.type = h.type;
    in.ext = h.ext;
    in.seat = SeatFromStick(stick_x_, stick_y_);
    in.key_pc = key_pc_;
    in.velocity = held_vel_;
    Voicing next{};
    Render(in, h.v.n ? &h.v : nullptr, &next);
    if (h.v.n == 0) {
        h.v = next;
        voicingOn(h.v, held_vel_);
        return;
    }
    diffVoicing(h.v, next, held_vel_);
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
    if (mode_ != PlayMode::Pro) return;
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
    if (mode_ != PlayMode::Pro) return;
    if (on) pad_ext_ = static_cast<uint8_t>(pad_ext_ | bit);
    else pad_ext_ = static_cast<uint8_t>(pad_ext_ & ~bit);
    refreshHeldChordExts();
}

void Session::degreePad(uint8_t idx, bool on) {
    if (mode_ != PlayMode::Smart) return;
    if (idx > 7) return;

    const uint8_t bit = static_cast<uint8_t>(1u << idx);
    if (on) {
        if (degree_held_ & bit) return;
        degree_held_ = static_cast<uint8_t>(degree_held_ | bit);
        if (findScaleDegree(idx)) return;
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
        h->ext = 0; // triad HOME; fancy 7ths = later setting
        h->v.n = 0;
        h->v.name[0] = 0;
        renderVoice(*h);
        return;
    }

    degree_held_ = static_cast<uint8_t>(degree_held_ & ~bit);
    Voice* h = findScaleDegree(idx);
    if (!h) return;
    voicingOff(h->v);
    h->root = -1;
    h->v.n = 0;
    h->scale = false;
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
}

void Session::stickCcX(uint8_t value) { stickCc(&stick_x_armed_, &stick_x_, value); }
void Session::stickCcY(uint8_t value) { stickCc(&stick_y_armed_, &stick_y_, value); }

void Session::setStickX(float x) {
    stick_x_armed_ = true;
    if (x < -1.f) x = -1.f;
    if (x > 1.f) x = 1.f;
    stick_x_ = x;
}

void Session::setStickY(float y) {
    stick_y_armed_ = true;
    if (y < -1.f) y = -1.f;
    if (y > 1.f) y = 1.f;
    stick_y_ = y;
}

void Session::setKeyHeld(bool held) { key_held_ = held; }

void Session::updateSeat() {
    Seat seat = SeatFromStick(stick_x_, stick_y_);
    if (seat == last_seat_) return;
    last_seat_ = seat;
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices_[i].root >= 0 && voices_[i].chord) renderVoice(voices_[i]);
    }
}

void Session::noteOn(uint8_t channel, uint8_t pitch, uint8_t velocity) {
    if (velocity == 0) {
        noteOff(channel, pitch);
        return;
    }
    out_ch_ = channel ? channel : 1;
    if (key_held_) {
        setKeyPc(static_cast<uint8_t>(pitch % 12));
        return;
    }

    // Smart mode: keyboard is optional. Unassigned notes pass thru as melody.
    if (mode_ == PlayMode::Smart) {
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
    pushOut(MidiEvent::Kind::Cc, 123, 0);
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

void Session::chordName(char* buf, size_t cap) const {
    if (!buf || cap == 0) return;
    buf[0] = 0;
    for (int i = 0; i < kMaxVoices; ++i) {
        if (voices_[i].root >= 0 && voices_[i].chord && voices_[i].v.name[0]) {
            std::strncpy(buf, voices_[i].v.name, cap - 1);
            buf[cap - 1] = 0;
            return;
        }
    }
}

} // namespace oc
