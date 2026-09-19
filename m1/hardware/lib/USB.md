# USB-C — Hroparts TYPE-C-31-M-12

| | |
|--|--|
| LCSC (buy) | [C165948](https://www.lcsc.com/product-detail/C165948.html) |
| KiCad | `oc-usb:TYPE-C-31-M-12` (symbol + footprint) |
| Rev A | **J1** on schematic and PCB |
| Files | `lib/easyeda/oc_usb.{kicad_sym,pretty,3dshapes}/` |
| In lib only | GCT `USB4105-GF-A` (do not place on rev A) |

USB 2.0 Type-C receptacle, right-angle. SMT signal pads + 4× through-hole shell stakes + 2× NPTH locating pegs.

## Rev A wiring (J1)

EasyEDA merges some VBUS/GND pairs. Pin numbers differ from GCT:

| Net | TYPE-C-31-M-12 pins |
|-----|---------------------|
| VBUS (5 V in) | **A4B9**, **B4A9** |
| GND | **A1B12**, **B1A12**, shell **EH 1–4** |
| D+ → series R → RP2040 USB_DP | **A6** (DP1) **and** **B6** (DP2) tied |
| D− → series R → RP2040 USB_DM | **A7** (DN1) **and** **B7** (DN2) tied |
| CC1 → 5.1 kΩ → GND | **A5** |
| CC2 → 5.1 kΩ → GND | **B5** |
| NC | **A8** (SBU1), **B8** (SBU2) |

Do **not** tie CC1 to CC2.

BOM qty: **1**. Second port not on rev A.

## Re-import

```bash
python3 -m easyeda2kicad --lcsc_id C165948 --full \
  --output /tmp/oc_usb_hro/oc_usb_hro.kicad_sym --overwrite
# rename footprint/3D USB-C_SMD-TYPE-C-31-M-12_1 → TYPE-C-31-M-12
# 3D → ${KIPRJMOD}/../lib/easyeda/oc_usb.3dshapes/TYPE-C-31-M-12.step
# locating pegs: NPTH (not plated)
```
