#include "screen.h"

#include <cstdio>
#include <cstring>

namespace ocfw {
namespace {

// 5×7, column-major, bit 0 is the top row. Missing glyphs draw nothing.
struct Glyph {
    char c;
    uint8_t col[5];
};

constexpr Glyph kFont[] = {
    {' ', {0, 0, 0, 0, 0}},
    {'#', {0x7F, 0x14, 0x7F, 0x14, 0x7F}},
    {'+', {0x08, 0x08, 0x3E, 0x08, 0x08}},
    {'-', {0x08, 0x08, 0x08, 0x08, 0x08}},
    {'.', {0x00, 0x60, 0x60, 0x00, 0x00}},
    {':', {0x00, 0x36, 0x36, 0x00, 0x00}},
    {'/', {0x20, 0x10, 0x08, 0x04, 0x02}},
    {'<', {0x08, 0x14, 0x22, 0x41, 0x00}},
    {'>', {0x00, 0x41, 0x22, 0x14, 0x08}},
    {'0', {0x3E, 0x51, 0x49, 0x45, 0x3E}},
    {'1', {0x00, 0x42, 0x7F, 0x40, 0x00}},
    {'2', {0x42, 0x61, 0x51, 0x49, 0x46}},
    {'3', {0x21, 0x41, 0x45, 0x4B, 0x31}},
    {'4', {0x18, 0x14, 0x12, 0x7F, 0x10}},
    {'5', {0x27, 0x45, 0x45, 0x45, 0x39}},
    {'6', {0x3C, 0x4A, 0x49, 0x49, 0x30}},
    {'7', {0x01, 0x71, 0x09, 0x05, 0x03}},
    {'8', {0x36, 0x49, 0x49, 0x49, 0x36}},
    {'9', {0x06, 0x49, 0x49, 0x29, 0x1E}},
    {'A', {0x7E, 0x11, 0x11, 0x11, 0x7E}},
    {'B', {0x7F, 0x49, 0x49, 0x49, 0x36}},
    {'C', {0x3E, 0x41, 0x41, 0x41, 0x22}},
    {'D', {0x7F, 0x41, 0x41, 0x22, 0x1C}},
    {'E', {0x7F, 0x49, 0x49, 0x49, 0x41}},
    {'F', {0x7F, 0x09, 0x09, 0x09, 0x01}},
    {'G', {0x3E, 0x41, 0x49, 0x49, 0x7A}},
    {'H', {0x7F, 0x08, 0x08, 0x08, 0x7F}},
    {'I', {0x00, 0x41, 0x7F, 0x41, 0x00}},
    {'J', {0x20, 0x40, 0x41, 0x3F, 0x01}},
    {'K', {0x7F, 0x08, 0x14, 0x22, 0x41}},
    {'L', {0x7F, 0x40, 0x40, 0x40, 0x40}},
    {'M', {0x7F, 0x02, 0x0C, 0x02, 0x7F}},
    {'N', {0x7F, 0x04, 0x08, 0x10, 0x7F}},
    {'O', {0x3E, 0x41, 0x41, 0x41, 0x3E}},
    {'P', {0x7F, 0x09, 0x09, 0x09, 0x06}},
    {'Q', {0x3E, 0x41, 0x51, 0x21, 0x5E}},
    {'R', {0x7F, 0x09, 0x19, 0x29, 0x46}},
    {'S', {0x46, 0x49, 0x49, 0x49, 0x31}},
    {'T', {0x01, 0x01, 0x7F, 0x01, 0x01}},
    {'U', {0x3F, 0x40, 0x40, 0x40, 0x3F}},
    {'V', {0x1F, 0x20, 0x40, 0x20, 0x1F}},
    {'W', {0x3F, 0x40, 0x38, 0x40, 0x3F}},
    {'X', {0x63, 0x14, 0x08, 0x14, 0x63}},
    {'Y', {0x07, 0x08, 0x70, 0x08, 0x07}},
    {'Z', {0x61, 0x51, 0x49, 0x45, 0x43}},
    {'a', {0x20, 0x54, 0x54, 0x54, 0x78}},
    {'b', {0x7F, 0x48, 0x44, 0x44, 0x38}},
    {'c', {0x38, 0x44, 0x44, 0x44, 0x20}},
    {'d', {0x38, 0x44, 0x44, 0x48, 0x7F}},
    {'e', {0x38, 0x54, 0x54, 0x54, 0x18}},
    {'f', {0x08, 0x7E, 0x09, 0x01, 0x02}},
    {'g', {0x0C, 0x52, 0x52, 0x52, 0x3E}},
    {'h', {0x7F, 0x08, 0x04, 0x04, 0x78}},
    {'i', {0x00, 0x44, 0x7D, 0x40, 0x00}},
    {'j', {0x20, 0x40, 0x44, 0x3D, 0x00}},
    {'k', {0x7F, 0x10, 0x28, 0x44, 0x00}},
    {'l', {0x00, 0x41, 0x7F, 0x40, 0x00}},
    {'m', {0x7C, 0x04, 0x18, 0x04, 0x78}},
    {'n', {0x7C, 0x08, 0x04, 0x04, 0x78}},
    {'o', {0x38, 0x44, 0x44, 0x44, 0x38}},
    {'p', {0x7C, 0x14, 0x14, 0x14, 0x08}},
    {'q', {0x08, 0x14, 0x14, 0x18, 0x7C}},
    {'r', {0x7C, 0x08, 0x04, 0x04, 0x08}},
    {'s', {0x48, 0x54, 0x54, 0x54, 0x20}},
    {'t', {0x04, 0x3F, 0x44, 0x40, 0x20}},
    {'u', {0x3C, 0x40, 0x40, 0x20, 0x7C}},
    {'v', {0x1C, 0x20, 0x40, 0x20, 0x1C}},
    {'w', {0x3C, 0x40, 0x30, 0x40, 0x3C}},
    {'x', {0x44, 0x28, 0x10, 0x28, 0x44}},
    {'y', {0x0C, 0x50, 0x50, 0x50, 0x3C}},
    {'z', {0x44, 0x64, 0x54, 0x4C, 0x44}},
};

const Glyph* FindGlyph(char c) {
    for (const Glyph& g : kFont) {
        if (g.c == c) return &g;
    }
    return nullptr;
}

void SetPixel(uint8_t* bitmap, int x, int y) {
    if (x < 0 || x >= kScreenW || y < 0 || y >= kScreenH) return;
    bitmap[(y / 8) * kScreenW + x] |= static_cast<uint8_t>(1u << (y % 8));
}

int TextWidth(const char* s, int scale) {
    if (!s || !s[0]) return 0;
    const int n = static_cast<int>(std::strlen(s));
    const int advance = (5 + 1) * scale;
    return n * advance - scale;
}

void DrawText(uint8_t* bitmap, const char* s, int x, int y, int scale, int clipL, int clipR) {
    if (!s) return;
    for (int i = 0; s[i]; ++i) {
        const Glyph* g = FindGlyph(s[i]);
        for (int col = 0; col < 5; ++col) {
            const uint8_t bits = g ? g->col[col] : 0;
            for (int row = 0; row < 7; ++row) {
                if ((bits & (1u << row)) == 0) continue;
                for (int sy = 0; sy < scale; ++sy) {
                    for (int sx = 0; sx < scale; ++sx) {
                        const int px = x + col * scale + sx;
                        if (px < clipL || px >= clipR) continue;
                        SetPixel(bitmap, px, y + row * scale + sy);
                    }
                }
            }
        }
        x += (5 + 1) * scale;
    }
}

void DrawCentered(uint8_t* bitmap, const char* s, int y, int scale, int clipL, int clipR) {
    const int w = TextWidth(s, scale);
    int x = clipL + (clipR - clipL - w) / 2;
    if (x < clipL) x = clipL;
    DrawText(bitmap, s, x, y, scale, clipL, clipR);
}

} // namespace

void drawScreen(const ScreenText& text, uint8_t bitmap[kScreenBytes]) {
    std::memset(bitmap, 0, kScreenBytes);
    DrawCentered(bitmap, text.top, 0, 2, 0, kScreenW);

    constexpr int kSplitA = 42;
    constexpr int kSplitB = 86;
    DrawCentered(bitmap, text.left, 18, 1, 1, kSplitA - 1);
    DrawCentered(bitmap, text.mid, 18, 1, kSplitA + 2, kSplitB - 1);
    DrawCentered(bitmap, text.right, 18, 1, kSplitB + 2, kScreenW - 1);
    for (int y = 18; y < 28; ++y) {
        SetPixel(bitmap, kSplitA, y);
        SetPixel(bitmap, kSplitB, y);
    }

    if (text.zones <= 0) return;
    const int cell = kScreenW / text.zones;
    for (int i = 0; i < text.zones; ++i) {
        const int x0 = i * cell + 2;
        const int x1 = (i + 1) * cell - 2;
        for (int x = x0; x < x1; ++x) {
            SetPixel(bitmap, x, 31);
            if (i == text.zone) SetPixel(bitmap, x, 30);
        }
    }
}

void drawLines(const char* const lines[4], uint8_t bitmap[kScreenBytes]) {
    std::memset(bitmap, 0, kScreenBytes);
    for (int i = 0; i < 4; ++i) {
        if (!lines[i] || !lines[i][0]) continue;
        DrawText(bitmap, lines[i], 0, i * 8, 1, 0, kScreenW);
    }
}

void drawCounts(const uint16_t ch[49], bool ok, uint8_t fingers, uint8_t bitmap[kScreenBytes]) {
    std::memset(bitmap, 0, kScreenBytes);
    char line[12];
    if (fingers > 0) std::snprintf(line, sizeof(line), "f %u", fingers);
    else std::snprintf(line, sizeof(line), "up");
    DrawText(bitmap, line, 40, 0, 1, 0, kScreenW);
    if (!ok || !ch) {
        DrawText(bitmap, "wait", 40, 12, 1, 0, kScreenW);
        return;
    }
    uint16_t ordered[49];
    std::memcpy(ordered, ch, sizeof(ordered));
    for (int i = 1; i < 49; ++i) {
        const uint16_t v = ordered[i];
        int j = i;
        while (j > 0 && ordered[j - 1] > v) {
            ordered[j] = ordered[j - 1];
            --j;
        }
        ordered[j] = v;
    }
    const uint16_t med = ordered[24];
    // The four corners sit outside the circle and read high with no finger.
    int hot = 3 * 7 + 3;
    for (int tx = 0; tx < 7; ++tx) {
        for (int rx = 0; rx < 7; ++rx) {
            if ((tx == 0 || tx == 6) && (rx == 0 || rx == 6)) continue;
            const int i = tx * 7 + rx;
            if (ch[i] < ch[hot]) hot = i;
        }
    }
    const int rise = med / 3;
    for (int tx = 0; tx < 7; ++tx) {
        for (int rx = 0; rx < 7; ++rx) {
            const uint16_t v = ch[tx * 7 + rx];
            const int x0 = tx * 4;
            const int y0 = rx * 4;
            const bool dead = v == 0 || (med > 20 && v + v < med);
            const bool high = v > med && static_cast<int>(v - med) > rise;
            if (dead) {
                SetPixel(bitmap, x0, y0);
                SetPixel(bitmap, x0 + 2, y0);
                SetPixel(bitmap, x0 + 1, y0 + 1);
                SetPixel(bitmap, x0, y0 + 2);
                SetPixel(bitmap, x0 + 2, y0 + 2);
            } else if (high) {
                for (int dy = 0; dy < 3; ++dy) {
                    for (int dx = 0; dx < 3; ++dx) SetPixel(bitmap, x0 + dx, y0 + dy);
                }
            } else {
                SetPixel(bitmap, x0 + 1, y0 + 1);
            }
        }
    }
    std::snprintf(line, sizeof(line), "R%d T%d", hot % 7, hot / 7);
    DrawText(bitmap, line, 40, 8, 1, 0, kScreenW);
    std::snprintf(line, sizeof(line), "%u", ch[hot]);
    DrawText(bitmap, line, 40, 16, 1, 0, kScreenW);
    std::snprintf(line, sizeof(line), "m %u", med);
    DrawText(bitmap, line, 40, 24, 1, 0, kScreenW);
}

} // namespace ocfw
