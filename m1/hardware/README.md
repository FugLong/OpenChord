# M1 hardware

RP2040 on our PCB, USB-C on the edge. Not a Pico carrier. **Hand-assemble only.** One board, no harnesses.

| Doc | What |
|-----|------|
| [bom.md](bom.md) | Parts + small-batch cost ballpark |
| [switches.md](switches.md) | Gateron LP, direct GPIO (no matrix) |
| [touch.md](touch.md) | Cap strip + wheel + system pads (2× QT2120) |
| [lib/OLED.md](lib/OLED.md) | OLED pinout / STEP |

Product locks: [`../docs/goals.md`](../docs/goals.md).

## Plan (short)

- **8× Gateron LP** chord pads (hotswap) — mechanical, **one GPIO each** (internal pull-ups; no diodes).
- **2× AT42QT2120:** strip (slider) + wheel (color stick) + spare SNS for Key/Shift/mode.
- Two I2C buses (fixed QT2120 addr). Alps stick + tactiles = **fallback only**.
- No SoftPot, no pots, no paid PCBA.

## KiCad `lib/`

| Path | Contents |
|------|----------|
| `3dshapes/` | Imported STEPs |
| `oled.pretty/` | 0.91" OLED |
| `lcsc.*` | Alps stick C219778 (**fallback**) |

Gateron LP hotswap: external footprint until schematic. Coupon before full fab.
