#pragma once

#include <cstddef>
#include <cstdint>

namespace ocfw {

// 128×32, SSD1306 page order: 4 pages × 128 columns, LSB is the top pixel of the page.
constexpr int kScreenW = 128;
constexpr int kScreenH = 32;
constexpr int kScreenBytes = kScreenW * kScreenH / 8;

struct ScreenText {
    char top[24]{};
    char left[16]{};
    char mid[16]{};
    char right[16]{};
    int zones = 0;
    int zone = -1;
};

// Same two lines the plugin paints. `zones` is 0 when the strip is idle.
void drawScreen(const ScreenText& text, uint8_t bitmap[kScreenBytes]);

// Four left-aligned rows, scale 1. Unused rows are null.
void drawLines(const char* const lines[4], uint8_t bitmap[kScreenBytes]);

// 7×7 electrode map, oriented like the pad. Left is Tx0, top is Rx0.
// A filled cell is well above the median. An X is a low cell, where a finger is.
void drawCounts(const uint16_t ch[49], bool ok, uint8_t fingers, uint8_t bitmap[kScreenBytes]);

} // namespace ocfw
