# M1 hardware

RP2040 on our PCB, USB-C on the edge. Not a Pico carrier. **Hand-assemble only** (no paid PCBA). One board, no harnesses.

| Doc | What |
|-----|------|
| [bom.md](bom.md) | Parts + packages + buy list |
| [switches.md](switches.md) | Gateron LP hotswap + 2×4 matrix |
| [lib/OLED.md](lib/OLED.md) | OLED footprint / pinout / STEP |

Product locks: [`../docs/goals.md`](../docs/goals.md).

## KiCad `lib/`

| Path | Contents |
|------|----------|
| `3dshapes/` | Imported STEP models (drop new ones here) |
| `oled.pretty/` | 0.91" OLED footprint |
| `lcsc.*` | Alps stick C219778 |

Gateron LP hotswap: external footprint until schematic (`switches.md`). Coupon before full fab.
