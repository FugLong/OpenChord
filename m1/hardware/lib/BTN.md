# Buttons — EasyEDA / LCSC imports

Nickname: **`oc-btn`**. Files: `lib/easyeda/oc_btn.{kicad_sym,pretty,3dshapes}/`.

| Part | Role | LCSC | Size | KiCad |
|------|------|------|------|-------|
| **EVQPUC02K** | System / edge (side-push) | [C79174](https://www.lcsc.com/product-detail/C79174.html) | 4.7×4.5×1.65, side | `oc-btn:EVQPUC02K` |
| **TS-1187A-B-A-B** | BOOTSEL / RUN (top-push) | [C318884](https://www.lcsc.com/product-detail/C318884.html) | **5.1×5.1×1.5 mm** | `oc-btn:TS-1187A-B-A-B` |

## EVQPUC02K (SW9 / SW10 / SW11)

Panasonic EVQ-PU side-press SPST. Four pads: **1+2** one pole, **3+4** the other. SMD pads 1.55×1.0 at x=±2.60, y=±0.85. Two **0.9 mm NPTH** locating bosses at (0, ±1.4).

Wiring: one pole → GPIO (MCU pull-up), other → GND. Roles TBD.

## TS-1187A-B-A-B (bring-up tactiles)

XKB Connection SPST-NO, 4-pad SMD, **top actuated**, low profile.

**Schematic (rev A):** Place `oc-btn:TS-1187A-B-A-B` as **SW13**. Four pads — **top 1+2** = one pole, **bottom 3+4** = other. Press bridges top↔bottom.

| Use | As built |
|-----|----------|
| **BOOTSEL (SW13)** | Pad **1 → GND**; pad **3 → R5 1 kΩ → `~QSPI_SS`**. Pads 2 and 4 NC. **R30 10 kΩ** always populated: `~QSPI_SS` → 3V3. |
| **RUN / RST** | **Not placed** on rev A. RP2040 RUN still has **R6 10 kΩ** pull-up to 3V3. |

## Re-import

```bash
python3 -m easyeda2kicad --lcsc_id C79174 C318884 --full \
  --output m1/hardware/lib/easyeda/oc_btn.kicad_sym --overwrite
# rename long EasyEDA names → EVQPUC02K / TS-1187A-B-A-B
# 3D → ${KIPRJMOD}/../lib/easyeda/oc_btn.3dshapes/<name>.step
# PUC bosses: pad type NPTH (not plated)
```
