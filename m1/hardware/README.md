# M1 hardware

RP2040 on our PCB, USB-C on the edge. Not a Pico carrier. **Hand-assemble only.** One board, no harnesses.

| Doc | What |
|-----|------|
| [bom.md](bom.md) | Parts + DigiKey/LCSC |
| [support.md](support.md) | Flash / clock / power / USB / MIDI recipe (place before wiring) |
| [switches.md](switches.md) | Gateron LP, direct GPIO (no matrix) |
| [touch.md](touch.md) | Cap strip + trackpad (**IQS572** + **QT2120**) |
| [lib/OLED.md](lib/OLED.md) | OLED pinout / STEP / symbol |
| [lib/TOUCH_ICS.md](lib/TOUCH_ICS.md) | IQS572 + QT2120 EasyEDA/LCSC imports |
| [lib/USB.md](lib/USB.md) | Hroparts TYPE-C-31-M-12 USB-C |
| [lib/TRS.md](lib/TRS.md) | Same Sky SJ1-3523N TRS MIDI |
| [lib/BTN.md](lib/BTN.md) | Panasonic EVQPUC02K edge |
| [lib/MIDI.md](lib/MIDI.md) | Toshiba TLP2361 MIDI IN opto |

Product locks: [`../docs/goals.md`](../docs/goals.md).

## Plan (short)

- **8× Gateron LP** chord pads (hotswap) — mechanical, **one GPIO each** (internal pull-ups; no diodes).
- **IQS572BLQNR** trackpad (color stick XY) + **AT42QT2120** strip only.
- **3× EVQPUC02K** system buttons → GPIO (**roles TBD**). Not capacitive.
- **BOOTSEL** = top tact **TS-1187A** → QSPI_SS through **1 kΩ**, plus **10 kΩ** SS→3V3. No RUN button on rev A.
- Shared **I2C1** on GPIO10/11 (OLED `0x3C`, QT2120 `0x1C`, IQS572 `0x74`; RDY GPIO8, NRST GPIO9). Alps stick = **fallback only**. Pin map: [`support.md`](support.md).
- No SoftPot, no pots, no paid PCBA. Coupon before a later product fab — see [`touch.md`](touch.md).

## Rev A fab (ordering)

| | |
|--|--|
| Size | **118.95 × 63.15 mm** |
| Stack | 4-layer, 1.6 mm, 1 oz. In1 **GND**, In2 **+3.3V**. SMT on **B.Cu**. |
| Finish | **HASL** (not ENIG). Touch = soldermask over copper. |
| Gerbers | [`fab/jlcpcb/`](fab/jlcpcb/) — zip `fab/OpenChordM1-gerbers.zip` (gitignored). Upload that zip. |
| Stencil | **Bottom only**, frameless, custom **160 × 120 mm**, 0.12 mm. No top stencil (`F_Paste` is empty). |
| TRS | SJ1-3523N bushings hang **off** Edge.Cuts so the barrel does not clip FR4. |

## KiCad `lib/`

| Path | Contents |
|------|----------|
| `oled.*` | OLED symbol + footprint + STEP ([OLED.md](lib/OLED.md)) |
| `gateron.*` | Gateron LP hotswap ([GATERON.md](lib/GATERON.md)) |
| `easyeda/oc_touch.*` | IQS572 + QT2120 ([TOUCH_ICS.md](lib/TOUCH_ICS.md)) |
| `easyeda/oc_usb.*` | TYPE-C-31-M-12 USB-C ([USB.md](lib/USB.md)); GCT kept in lib |
| `easyeda/oc_trs.*` | Same Sky SJ1-3523N ([TRS.md](lib/TRS.md)) |
| `easyeda/oc_btn.*` | EVQPUC02K edge + TS-1187A BOOT/RUN ([BTN.md](lib/BTN.md)) |
| `easyeda/oc_midi.*` | Toshiba TLP2361 ([MIDI.md](lib/MIDI.md)) |
| `captouch.*` | Generated electrodes (slider/trackpad) — plugin writes here, not into `OpenChordM1/` |
| `lcsc.*` | Alps stick C219778 (**fallback**) |
| `3dshapes/` | OLED STEP |

Nicknames: `oc-oled`, `oc-lcsc`, `oc-gateron`, `oc-touch`, `oc-usb`, `oc-trs`, `oc-btn`, `oc-midi`, `captouch`. RP2040 = KiCad stock `MCU_RaspberryPi` + `Package_DFN_QFN`. Place slider as `captouch:CT_Slider`. Support ICs (flash **W25Q32JVSSIQ** / [C179173](https://www.lcsc.com/product-detail/C179173.html), LDO/ESD/crystal/diode) = KiCad stock — see [`support.md`](support.md).
