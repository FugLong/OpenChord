# TRS MIDI — Same Sky SJ1-3523N

| | |
|--|--|
| DigiKey | [SJ1-3523N](https://www.digikey.com/en/products/detail/same-sky-formerly-cui-devices/SJ1-3523N/738689) |
| LCSC (EasyEDA CAD) | [C20182914](https://www.lcsc.com/product-detail/C20182914.html) |
| KiCad | `oc-trs:SJ1-3523N` |
| Files | `lib/easyeda/oc_trs.{kicad_sym,pretty,3dshapes}/` |

3.5 mm stereo jack, right-angle TH, **0 internal switches**. Qty **2** (MIDI IN + MIDI OUT).

## Pins (Same Sky datasheet)

| Pad | Contact | Type A MIDI |
|-----|---------|-------------|
| 1 | Sleeve | GND / shield |
| 2 | Tip | Current source (+) |
| 3 | Ring | Current sink (−) |

## Schematic notes

- **OUT:** UART TX → series R → Tip (2); Ring (3) → return / −; Sleeve → GND. Leave hot with no plug.
- **IN:** Tip/Ring → H11L1M (or equiv) optocoupler; Sleeve → GND. No plug = idle.
- Mounting pads (unnumbered) → GND if you want shell bonded.
