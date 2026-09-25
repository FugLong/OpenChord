#include "chord_engine.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace oc {
namespace {

const char* kPcName[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

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
        case Type::Sus2:
            AddUnique(ivs, n, 0);
            AddUnique(ivs, n, 2);
            AddUnique(ivs, n, 7);
            break;
        case Type::None:
            return;
    }

    // Keyswitch extras are literal: M7 means M7, 6 means 6.
    if (in.ext & Ext6) AddUnique(ivs, n, 9);
    if (in.ext & Extm7) AddUnique(ivs, n, 10);
    if (in.ext & ExtM7) AddUnique(ivs, n, 11);
    if (in.ext & Ext9) AddUnique(ivs, n, 14);
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
        case Type::Sus2: q = "sus2"; break;
        default: q = ""; break;
    }
    char extra[8] = {0};
    int e = 0;
    if (in.type == Type::Dim && (in.ext & Extm7)) {
        q = "m7b5";
    } else if (in.ext & Ext6) extra[e++] = '6';
    if (in.ext & Extm7) {
        if (in.type != Type::Dim) extra[e++] = '7';
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

    int inv = in.color ? static_cast<int>(in.color_inv) : InversionIndex(in.seat);
    if (inv < 0) inv = 0;
    if (inv >= n) inv = n - 1;
    const bool open = in.color ? in.color_open : IsOpen(in.seat);
    Place(ivs, n, inv, open, in.root_midi, prev, out->notes);
    out->n = static_cast<uint8_t>(n);
    MakeName(in, out->name, sizeof(out->name));
    if (in.color && inv > 0) {
        const int bass = (static_cast<int>(in.root_midi) + ivs[inv]) % 12;
        const size_t used = strlen(out->name);
        snprintf(out->name + used, sizeof(out->name) - used, "/%s", kPcName[bass]);
    }
    if (in.color && in.color_open) {
        const size_t used = strlen(out->name);
        snprintf(out->name + used, sizeof(out->name) - used, " open");
    }
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

bool NameHeardChord(const uint8_t* notes, int n, char* out, size_t cap) {
    if (!out || cap == 0) return false;
    out[0] = 0;
    if (!notes || n < 2) return false;

    uint16_t pcs = 0;
    int bass = 127;
    for (int i = 0; i < n; ++i) {
        const int note = notes[i];
        if (note < 0 || note > 127) continue;
        pcs = static_cast<uint16_t>(pcs | (1u << (note % 12)));
        if (note < bass) bass = note;
    }
    if (bass > 127) return false;
    int kinds = 0;
    for (int i = 0; i < 12; ++i)
        if (pcs & (1u << i)) ++kinds;
    if (kinds < 2) return false;
    const int bassPc = bass % 12;

    // Intervals from the root. The 5th may be missing when a shape lists it after a comma...
    // `need` must all be present. `allow` is every interval the shape may include.
    struct Shape {
        const char* suf;
        const char* need;
        const char* allow;
        int rank;
    };
    static const Shape kShapes[] = {
        {"13", "0 4 9 10", "0 2 4 7 9 10", 90},
        {"maj13", "0 4 9 11", "0 2 4 7 9 11", 90},
        {"m13", "0 3 9 10", "0 2 3 7 9 10", 88},
        {"9", "0 2 4 10", "0 2 4 7 10", 86},
        {"maj9", "0 2 4 11", "0 2 4 7 11", 86},
        {"m9", "0 2 3 10", "0 2 3 7 10", 86},
        {"7b9", "0 1 4 10", "0 1 4 7 10", 84},
        {"7#9", "0 3 4 10", "0 3 4 7 10", 84},
        {"7#11", "0 4 6 10", "0 4 6 7 10", 84},
        {"7b13", "0 4 8 10", "0 4 7 8 10", 82},
        {"11", "0 2 5 10", "0 2 4 5 7 10", 74},
        {"m11", "0 2 3 5 10", "0 2 3 5 7 10", 80},
        {"6/9", "0 2 4 9", "0 2 4 7 9", 78},
        {"9sus", "0 2 5 10", "0 2 5 7 10", 76},
        {"dim7", "0 3 6 9", "0 3 6 9", 74},
        {"m7b5", "0 3 6 10", "0 3 6 10", 72},
        {"mMaj7", "0 3 11", "0 3 7 11", 70},
        {"augM7", "0 4 8 11", "0 4 8 11", 70},
        {"aug7", "0 4 8 10", "0 4 8 10", 68},
        {"7sus", "0 5 10", "0 5 7 10", 66},
        {"M7", "0 4 11", "0 4 7 11", 64},
        {"7", "0 4 10", "0 4 7 10", 62},
        {"m7", "0 3 10", "0 3 7 10", 62},
        {"6", "0 4 9", "0 4 7 9", 58},
        {"m6", "0 3 9", "0 3 7 9", 58},
        {"add9", "0 2 4", "0 2 4 7", 54},
        {"madd9", "0 2 3", "0 2 3 7", 54},
        {"aug", "0 4 8", "0 4 8", 50},
        {"dim", "0 3 6", "0 3 6", 50},
        {"sus2", "0 2 7", "0 2 7", 48},
        {"sus", "0 5 7", "0 5 7", 48},
        {"m", "0 3 7", "0 3 7", 40},
        {"", "0 4 7", "0 4 7", 40},
        {"5", "0 7", "0 7", 20},
    };
    auto bits = [](const char* ivs) {
        uint16_t m = 0;
        for (const char* p = ivs; *p; ++p) {
            if (*p < '0' || *p > '9') continue;
            int v = *p - '0';
            if (p[1] >= '0' && p[1] <= '9') {
                v = v * 10 + (p[1] - '0');
                ++p;
            }
            if (v >= 0 && v < 12) m = static_cast<uint16_t>(m | (1u << v));
        }
        return m;
    };

    int bestScore = -1;
    int bestRoot = 0;
    const Shape* best = nullptr;
    for (int root = 0; root < 12; ++root) {
        uint16_t rel = 0;
        for (int pc = 0; pc < 12; ++pc) {
            if (pcs & (1u << pc))
                rel = static_cast<uint16_t>(rel | (1u << ((pc - root + 12) % 12)));
        }
        for (const Shape& s : kShapes) {
            const uint16_t need = bits(s.need);
            const uint16_t allow = bits(s.allow);
            if ((rel & need) != need) continue;
            if ((rel & static_cast<uint16_t>(~allow)) != 0) continue;
            const int score = s.rank + (root == bassPc ? 1000 : 0);
            if (score > bestScore) {
                bestScore = score;
                bestRoot = root;
                best = &s;
            }
        }
    }
    if (!best) return false;
    std::snprintf(out, cap, "%s%s", kPcName[bestRoot], best->suf);
    if (bestRoot != bassPc) {
        const size_t used = std::strlen(out);
        std::snprintf(out + used, cap - used, "/%s", kPcName[bassPc]);
    }
    return out[0] != 0;
}

} // namespace oc
