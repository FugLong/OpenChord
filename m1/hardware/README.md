# M1 hardware

RP2040 on our PCB, USB-C on the edge. Not a Pico carrier. **Hand-assemble only.** One board, no harnesses.

| Doc | What |
|-----|------|
| [bom.md](bom.md) | Parts + small-batch cost ballpark |
| [switches.md](switches.md) | Gateron LP, direct GPIO (no matrix) |
| [touch.md](touch.md) | Cap strip + trackpad + Key/Shift (**IQS572** + **QT2120**) |
| [lib/OLED.md](lib/OLED.md) | OLED pinout / STEP / symbol |
| [lib/TOUCH_ICS.md](lib/TOUCH_ICS.md) | IQS572 + QT2120 EasyEDA/LCSC imports |
| [lib/USB.md](lib/USB.md) | GCT USB4105-GF-A USB-C |
| [lib/TRS.md](lib/TRS.md) | Same Sky SJ1-3523N TRS MIDI |
| [lib/BTN.md](lib/BTN.md) | Panasonic EVQ-PUA02K edge mode |

Product locks: [`../docs/goals.md`](../docs/goals.md).

## Plan (short)

- **8× Gateron LP** chord pads (hotswap) — mechanical, **one GPIO each** (internal pull-ups; no diodes).
- **IQS572BLQNR** trackpad (color stick XY) + **AT42QT2120** strip / Key / Shift.
- Shared **I2C0** (OLED `0x3C`, QT2120 `0x1C`, IQS572 `0x74` + RDY GPIO). Mode = edge **EVQ-PUA02K**. Alps / Key-Shift tactiles = **fallback only**.
- No SoftPot, no pots, no paid PCBA. Coupon before full fab — see [`touch.md`](touch.md).

## KiCad `lib/`

| Path | Contents |
|------|----------|
| `oled.*` | OLED symbol + footprint + STEP ([OLED.md](lib/OLED.md)) |
| `gateron.*` | Gateron LP hotswap ([GATERON.md](lib/GATERON.md)) |
| `easyeda/oc_touch.*` | IQS572 + QT2120 ([TOUCH_ICS.md](lib/TOUCH_ICS.md)) |
| `easyeda/oc_usb.*` | GCT USB4105-GF-A ([USB.md](lib/USB.md)) |
| `easyeda/oc_trs.*` | Same Sky SJ1-3523N ([TRS.md](lib/TRS.md)) |
| `easyeda/oc_btn.*` | Panasonic EVQ-PUA02K ([BTN.md](lib/BTN.md)) |
| `captouch.*` | Generated electrodes (slider/trackpad) — plugin writes here, not into `OpenChordM1/` |
| `lcsc.*` | Alps stick C219778 (**fallback**) |
| `3dshapes/` | OLED STEP |

Nicknames: `oc-oled`, `oc-lcsc`, `oc-gateron`, `oc-touch`, `oc-usb`, `oc-trs`, `oc-btn`, `captouch`. RP2040 = KiCad stock `MCU_RaspberryPi` + `Package_DFN_QFN`. Place slider as `captouch:CT_Slider`.
