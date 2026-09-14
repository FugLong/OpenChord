# M1 hardware

RP2040 on our PCB, USB-C on the edge. Not a Pico carrier. **Hand-assemble only.** One board, no harnesses.

| Doc | What |
|-----|------|
| [bom.md](bom.md) | Parts |
| [switches.md](switches.md) | Gateron LP + matrix |
| [touch.md](touch.md) | Cap strip (tentative) |
| [lib/OLED.md](lib/OLED.md) | OLED pinout / STEP |

Product locks: [`../docs/goals.md`](../docs/goals.md).

## KiCad `lib/`

| Path | Contents |
|------|----------|
| `3dshapes/` | Imported STEPs |
| `oled.pretty/` | 0.91" OLED |
| `lcsc.*` | Alps stick C219778 |

Gateron LP hotswap: external footprint until schematic. Coupon before full fab.
