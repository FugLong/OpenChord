#include "chord_engine.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace oc {
namespace {

const char* kPcName[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

// Major scale steps from tonic.
const int8_t kMaj[7] = {0, 2, 4, 5, 7, 9, 11};

bool InKey(uint8_t pc, uint8_t key_pc) {
    int rel = (static_cast<int>(pc) - static_cast<int>(key_pc) + 12) % 12;
    for (int i = 0; i < 7; ++i) {
        if (kMaj[i] == rel) return true;
    }
    return false;
}

int8_t InKeyInterval(uint8_t /*key_pc*/, int want) {
    // want = 9, 11, 13 as scale-ish chord tones from root; clamp to 0-11 class later.
    // Natural 9/11/13: 14, 17, 21 semitones → pc 2, 5, 9 from root.
    if (want <= 9) return 14;
    if (want <= 11) return 17;
    return 21;
}

int8_t PreferInKey(int8_t iv, uint8_t root_pc, uint8_t key_pc) {
    uint8_t pc = static_cast<uint8_t>((root_pc + (iv % 12) + 12) % 12);
    if (InKey(pc, key_pc)) return iv;
    // Try flat then sharp of that extension.
    int8_t flat = static_cast<int8_t>(iv - 1);
    pc = static_cast<uint8_t>((root_pc + (flat % 12) + 12) % 12);
    if (InKey(pc, key_pc)) return flat;
    int8_t sharp = static_cast<int8_t>(iv + 1);
    pc = static_cast<uint8_t>((root_pc + (sharp % 12) + 12) % 12);
    if (InKey(pc, key_pc)) return sharp;
    return iv; // player asked via a button; keep it
}

void AddUnique(int8_t* ivs, int& n, int8_t v) {
    v = static_cast<int8_t>(((v % 12) + 12) % 12);
    for (int i = 0; i < n; ++i) {
        if (ivs[i] == v) return;
    }
    if (n < 8) ivs[n++] = v;
}

void BuildIntervals(const EngineInput& in, int8_t* ivs, int& n) {
    n = 0;
    switch (in.type) {
        case Type::Maj:
            AddUnique(ivs, n, 0);
            AddUnique(ivs, n, 4);
            AddUnique(ivs, n, 7);
            break;
        case Type::Min:
            AddUnique(ivs, n, 0);
            AddUnique(ivs, n, 3);
            AddUnique(ivs, n, 7);
            break;
        case Type::Dim:
            AddUnique(ivs, n, 0);
            AddUnique(ivs, n, 3);
            AddUnique(ivs, n, 6);
            break;
        case Type::Sus:
            AddUnique(ivs, n, 0);
            AddUnique(ivs, n, 5);
            AddUnique(ivs, n, 7);
            break;
        case Type::None:
            return;
    }

    uint8_t root_pc = static_cast<uint8_t>(in.root_midi % 12);
    // Pad extras are literal: M7 means M7, 6 means 6. Stick extras stay in key.
    if (in.ext & Ext6) AddUnique(ivs, n, 9);
    if (in.ext & Extm7) AddUnique(ivs, n, 10);
    if (in.ext & ExtM7) AddUnique(ivs, n, 11);
    if (in.ext & Ext9) AddUnique(ivs, n, 14);

    // Stick up: extra in-key color. Down: no extras (closed). Corners mix.
    int extra = 0;
    bool open = false;
    switch (in.seat) {
        case Seat::N:
        case Seat::NE:
        case Seat::NW:
            extra = (in.seat == Seat::N) ? 2 : 1;
            open = true;
            break;
        case Seat::S:
        case Seat::SE:
        case Seat::SW:
            extra = 0;
            open = false;
            break;
        case Seat::E:
        case Seat::W:
            extra = 0;
            open = false;
            break;
        case Seat::Home:
        default:
            extra = 0;
            open = false;
            break;
    }

    if (extra >= 1 && !(in.ext & Ext9)) {
        AddUnique(ivs, n, PreferInKey(InKeyInterval(in.key_pc, 9), root_pc, in.key_pc));
    }
    if (extra >= 2) {
        AddUnique(ivs, n, PreferInKey(InKeyInterval(in.key_pc, 11), root_pc, in.key_pc));
    }
    (void)open;
}

int InversionIndex(Seat s) {
    switch (s) {
        case Seat::E:
        case Seat::NE:
        case Seat::SE:
            return 1;
        case Seat::W:
        case Seat::NW:
        case Seat::SW:
            return 2;
        default:
            return 0;
    }
}

bool IsOpen(Seat s) {
    return s == Seat::N || s == Seat::NE || s == Seat::NW;
}

void Place(int8_t* ivs, int n, int inv, bool open, int root_midi, const Voicing* prev,
           uint8_t* notes) {
    // Rotate for inversion: bass is ivs[inv].
    int8_t rot[8];
    for (int i = 0; i < n; ++i) rot[i] = ivs[(i + inv) % n];

    int base = root_midi;
    if (base < 0) base = 0;
    if (base > 127) base = 127;

    for (int i = 0; i < n; ++i) {
        int pc = (root_midi + rot[i]) % 12;
        int midi = base - (base % 12) + pc;
        while (midi < base) midi += 12;
        if (i > 0 && midi <= notes[i - 1]) midi += 12;
        if (open && i > 0 && i < n - 1) midi += 12;
        while (midi > 127) midi -= 12;
        while (midi < 0) midi += 12;
        notes[i] = static_cast<uint8_t>(midi);
    }

    if (!prev || prev->n == 0) return;

    // Pull each new note toward nearest previous pitch of any voice.
    for (int i = 0; i < n; ++i) {
        int best = notes[i];
        int best_d = 128;
        for (int k = -2; k <= 2; ++k) {
            int cand = static_cast<int>(notes[i]) + k * 12;
            if (cand < 0 || cand > 127) continue;
            for (uint8_t p = 0; p < prev->n; ++p) {
                int d = cand - static_cast<int>(prev->notes[p]);
                if (d < 0) d = -d;
                if (d < best_d) {
                    best_d = d;
                    best = cand;
                }
            }
        }
        notes[i] = static_cast<uint8_t>(best);
    }

    // Restore ascending order without dropping pitch classes.
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (notes[j] < notes[i]) {
                uint8_t t = notes[i];
                notes[i] = notes[j];
                notes[j] = t;
            }
        }
    }
    for (int i = 1; i < n; ++i) {
        while (notes[i] <= notes[i - 1] && notes[i] <= 115) {
            notes[i] = static_cast<uint8_t>(notes[i] + 12);
        }
        while (notes[i] > 127) notes[i] = static_cast<uint8_t>(notes[i] - 12);
    }
}

void MakeName(const EngineInput& in, char* name, size_t cap) {
    uint8_t pc = static_cast<uint8_t>(in.root_midi % 12);
    const char* q = "";
    switch (in.type) {
        case Type::Maj: q = ""; break;
        case Type::Min: q = "m"; break;
        case Type::Dim: q = "dim"; break;
        case Type::Sus: q = "sus"; break;
        default: q = ""; break;
    }
    char extra[8] = {0};
    int e = 0;
    if (in.ext & Ext6) extra[e++] = '6';
    if (in.ext & Extm7) {
        extra[e++] = '7';
    } else if (in.ext & ExtM7) {
        extra[e++] = 'M';
        extra[e++] = '7';
    }
    if (in.ext & Ext9) extra[e++] = '9';
    extra[e] = 0;
    snprintf(name, cap, "%s%s%s", kPcName[pc], q, extra);
}

} // namespace

Seat SeatFromStick(float x, float y, float dead) {
    float m2 = x * x + y * y;
    if (m2 < dead * dead) return Seat::Home;
    // y positive = musical up (caller inverts hardware if needed)
    float a = atan2f(y, x); // -pi..pi, 0 = east
    float deg = a * 180.0f / 3.14159265f;
    if (deg < 0) deg += 360.0f;
    // 8 slices, E at 0°, offset 22.5 so cardinals are centered.
    int idx = static_cast<int>(floorf((deg + 22.5f) / 45.0f)) % 8;
    switch (idx) {
        case 0: return Seat::E;
        case 1: return Seat::NE;
        case 2: return Seat::N;
        case 3: return Seat::NW;
        case 4: return Seat::W;
        case 5: return Seat::SW;
        case 6: return Seat::S;
        default: return Seat::SE;
    }
}

void Render(const EngineInput& in, const Voicing* prev, Voicing* out) {
    out->n = 0;
    out->name[0] = 0;
    if (in.root_midi < 0 || in.type == Type::None) return;

    int8_t ivs[8];
    int n = 0;
    BuildIntervals(in, ivs, n);
    if (n == 0) return;

    int inv = InversionIndex(in.seat);
    if (inv >= n) inv = n - 1;
    Place(ivs, n, inv, IsOpen(in.seat), in.root_midi, prev, out->notes);
    out->n = static_cast<uint8_t>(n);
    MakeName(in, out->name, sizeof(out->name));
}

bool DegreeToChord(uint8_t key_pc, uint8_t degree_idx, int16_t tonic_midi,
                   int16_t* root_midi_out, Type* type_out) {
    if (!root_midi_out || !type_out) return false;
    if (degree_idx > 7) return false;

    // Major-key diatonic: I ii iii IV V vi vii°
    static const int8_t kDegPc[7] = {0, 2, 4, 5, 7, 9, 11};
    static const Type   kDegType[7] = {
        Type::Maj, Type::Min, Type::Min, Type::Maj, Type::Maj, Type::Min, Type::Dim};

    const uint8_t deg = (degree_idx == 7) ? 0 : degree_idx;
    int root = static_cast<int>(tonic_midi) + kDegPc[deg];
    if (degree_idx == 7) root += 12;
    while (root < 0) root += 12;
    while (root > 127) root -= 12;
    // Keep pitch class aligned to key + degree (tonic_midi should already be key_pc).
    (void)key_pc;
    *root_midi_out = static_cast<int16_t>(root);
    *type_out = kDegType[deg];
    return true;
}

} // namespace oc
