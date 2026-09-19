# TRS MIDI — Same Sky SJ1-3523N

| | |
|--|--|
| DigiKey | [SJ1-3523N](https://www.digikey.com/en/products/detail/same-sky-formerly-cui-devices/SJ1-3523N/738689) |
| LCSC (EasyEDA CAD) | [C20182914](https://www.lcsc.com/product-detail/C20182914.html) |
| KiCad | `oc-trs:SJ1-3523N` |
| Files | `lib/easyeda/oc_trs.{kicad_sym,pretty,3dshapes}/` |

3.5 mm stereo jack, right-angle TH, **0 internal switches**. Qty **2**: **J2 OUT**, **J3 IN** on **B.Cu**. The barrel OD is taller than the body — hang the bushing **off the board edge** (~3 mm) so it does not clip FR4. Do not dremel a notch.

## Pins (Same Sky datasheet)

| Pad | Contact | Type A MIDI |
|-----|---------|-------------|
| 1 | Sleeve | GND / shield |
| 2 | Tip | Current source (+) |
| 3 | Ring | Current sink (−) |

## Schematic notes

- **OUT (Type A):** Tip (2) ← **R12 33 Ω** ← 3V3 (current source +); Ring (3) ← **R11 10 Ω** ← UART0 TX GPIO0 (current sink −); Sleeve → GND. Leave hot with no plug.
- **IN:** Tip → **220 Ω** → **TLP2361** anode; Ring → cathode; **1N4148** across LED; VO → UART0 RX GPIO1; VCC=3V3 + 100 nF. Sleeve → GND. KiCad `oc-midi:TLP2361`. See [`MIDI.md`](MIDI.md) / [`../support.md`](../support.md).
- Unnumbered pads are locating bosses. On rev A they have **no net** (not tied to GND).
