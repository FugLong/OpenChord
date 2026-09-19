# Touch ICs — EasyEDA / LCSC imports

Imported with `easyeda2kicad` from LCSC. Nickname: **`oc-touch`**.

| Part | Order / source | Package | Symbol | Footprint | 3D |
|------|----------------|---------|--------|-----------|-----|
| **IQS572-BL-QNR** | DigiKey [7165004](https://www.digikey.com/en/products/detail/azoteq-pty-ltd/IQS572BLQNR/7165004) · LCSC [C3827635](https://www.lcsc.com/product-detail/C3827635.html) | QFN-28 4×4 **0.5 mm** pitch | `oc-touch:IQS572-BL-QNR` | `oc-touch:QFN-28_L4.0-W4.0-P0.50-TL` | STEP+WRL |
| **AT42QT2120-XUR** | DigiKey [3678735](https://www.digikey.com/en/products/detail/microchip-technology/AT42QT2120-XUR/3678735) · LCSC [C1522278](https://www.lcsc.com/product-detail/C1522278.html) (XU tube listing — same die/pkg) | TSSOP-20 | `oc-touch:AT42QT2120-XUR` | `oc-touch:TSSOP-20_L6.5-W4.4-P0.65-LS6.4-BL` | STEP+WRL |

Files under `lib/easyeda/oc_touch.{kicad_sym,pretty,3dshapes}/`.

## Notes

- DigiKey buy for IQS572 is **IQS572BLQNR**; LCSC # is for EasyEDA CAD + optional JLCPCB.
- DigiKey buy for QT2120 is **AT42QT2120-XUR** (TSSOP reel — better stock). Electrically identical to `-XU` tube. MMHR (VQFN 3×3) remains DigiKey-only alt — no EasyEDA import here.
- QT2120 pin names use LCSC **KEY0…** labels; those are the SNS electrodes (KEY0 = SNS0 = slider ch 0). Product sch uses **KEY0–2 only**; KEY3–11 open.
- IQS572 pinout matches Azoteq datasheet (SDA/SCL/VDDHI/VSS/VREG/NRST/RDY/Rx*/Tx*/PGM/SW_IN).
- QFN pitch **0.5 mm** confirmed against IQS5xx datasheet §11.3 (`e = 0.5`).
- EasyEDA footprint was **missing the exposed pad** — we added **pad 29 (EP)** (~2.7×2.7 mm). Tie **EP → VSS/GND** on the schematic (Azoteq recommends soldering EP to ground).

## Re-import

```bash
python3 -m easyeda2kicad --lcsc_id C3827635 C1522278 --full \
  --output m1/hardware/lib/easyeda/oc_touch.kicad_sym --overwrite
# then rename QT2120 symbol AT42QT2120-XU → AT42QT2120-XUR
# and re-point 3D models to ${KIPRJMOD}/../lib/easyeda/oc_touch.3dshapes/*.step
```
