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

} // namespace ocfw
