# Buttons — EasyEDA / LCSC imports

Nickname: **`oc-btn`**. Files: `lib/easyeda/oc_btn.{kicad_sym,pretty,3dshapes}/`.

| Part | Role | LCSC | Size | KiCad |
|------|------|------|------|-------|
| **EVQ-PUA02K** | System / edge (side-push) | [C128539](https://www.lcsc.com/product-detail/C128539.html) · [DigiKey](https://www.digikey.com/en/products/detail/panasonic-industry/EVQ-PUA02K/286334) | ~4.7×3.5, side | `oc-btn:EVQ-PUA02K` |
| **TS-1187A-B-A-B** | BOOTSEL / RUN (top-push) | [C318884](https://www.lcsc.com/product-detail/C318884.html) | **5.1×5.1×1.5 mm**, ~**$0.03–0.05** | `oc-btn:TS-1187A-B-A-B` |

## TS-1187A-B-A-B (bring-up tactiles)

XKB Connection SPST-NO, 4-pad SMD, **top actuated**, low profile. Same family every cheap MCU board uses for Boot/Reset.

**Schematic:** Place `oc-btn:TS-1187A-B-A-B`. Four pads — left column **1+3** = one pole (internally common), right **2+4** = other. Wire one pole → net, other → GND (tie both pads of each pole on the PCB).

| Use | Net |
|-----|-----|
| **BOOTSEL** | Through **1 kΩ** onto `~QSPI_SS` / flash CS; other side GND |
| **RUN / RST** (optional) | RP2040 **RUN** (active low reset); other side GND. Keep RUN’s **10 kΩ** pull-up to 3V3 |

## EVQ-PUA02K (system edge)

SMD side-push. Body ~4.7×3.5 mm, height off board 1.65 mm. Edge-mounted for the three TBD system buttons.

Schematic: one pin → GPIO (internal pull-up), other → GND.

## Re-import

```bash
python3 -m easyeda2kicad --lcsc_id C128539 C318884 --full \
  --output m1/hardware/lib/easyeda/oc_btn.kicad_sym --overwrite
# rename long EasyEDA footprint → TS-1187A-B-A-B; 3D →
# ${KIPRJMOD}/../lib/easyeda/oc_btn.3dshapes/TS-1187A-B-A-B.step
```
