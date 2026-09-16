# USB-C — GCT USB4105-GF-A

| | |
|--|--|
| DigiKey | [USB4105-GF-A](https://www.digikey.com/en/products/detail/gct/USB4105-GF-A/11198441) |
| LCSC (EasyEDA CAD) | [C3020560](https://www.lcsc.com/product-detail/C3020560.html) |
| KiCad | `oc-usb:USB4105-GF-A` |
| Files | `lib/easyeda/oc_usb.{kicad_sym,pretty,3dshapes}/` |

USB 2.0 Type-C receptacle, right-angle, SMT signals + TH shell pegs. Rated **20k** mating cycles (GCT series).

## Schematic (USB device / UFP)

- Tie **DP1↔DP2**, **DN1↔DN2** → RP2040 USB (± series R / ESD)
- All **VBUS** → 5 V in
- All **GND** + shell **EH** (pads 1–4) → GND
- **CC1** and **CC2** each → **5.1 kΩ → GND**
- **SBU1/SBU2** → NC

BOM qty: **1** (USB device / power). Second port not on rev A.
