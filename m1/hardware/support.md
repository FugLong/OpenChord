# M1 support circuit — as built (rev A schematic)

Companion to [`bom.md`](bom.md). **Source of truth is the KiCad schematic.** This file matches that netlist.

Paths: `OpenChordM1/OpenChordM1.kicad_sch` · `OpenChordM1.kicad_pcb`.

---

## Completeness (on schematic)

| Block | On sch |
|-------|--------|
| MCU | RP2040 |
| Boot flash | W25Q32JVSSIQ ([C179173](https://www.lcsc.com/product-detail/C179173.html)) QSPI |
| Clock | ABM8-272-T3 + 15 pF ×2 + 1 kΩ on XOUT |
| 3V3 | AP2112K-3.3, EN tied to VIN; 10 µF in/out |
| USB | J1 TYPE-C-31-M-12 ([C165948](https://www.lcsc.com/product-detail/C165948.html)) + USBLC6 + 22 Ω + CC 5.1 kΩ→GND |
| MIDI OUT | Type A: 33 Ω (3V3→Tip), 10 Ω (TX→Ring) |
| MIDI IN | TLP2361 + 220 Ω + 1N4148 + 100 nF |
| Touch | IQS572 + QT2120 + electrodes |
| Display | OLED 0.91" I²C |
| Keys | 8× Gateron LP → GPIO |
| System btns | EVQPUC02K ×3 → GPIO (**roles TBD**) |
| BOOTSEL | TS-1187A + 1 kΩ to `~QSPI_SS`; **10 kΩ pull-up SS→3V3** |
| RUN pin | 10 kΩ → 3V3 (no RUN button on rev A) |
| I²C | **I2C1** GPIO10/11, 4.7 kΩ ×2 |
| RP2040 ties | VREG 1 µF ×2; IO/DVDD 100 nF; **TESTEN→GND**; ADC_AVDD→3V3 |

**Not on rev A:** second USB-C, Alps stick, RUN tact, QT2120 Key/Shift copper, CHANGE GPIO.

**3× EVQ** = system buttons → GPIO. **BOOTSEL** = top tact `TS-1187A-B-A-B` (not the edge PUC). QT2120 SNS is strip only (KEY0–2). Do not wire SNS to buttons.

---

## 1. Power

```
USB VBUS ──► AP2112K VIN ──► +3V3 ──► RP2040 IOVDD / USB_VDD / VREG_IN / ADC_AVDD
                 EN ← VIN                 flash, OLED, QT2120, IQS572 VDDHI, TLP2361 VCC
```

| Cap | Where |
|-----|--------|
| C10 10 µF | VBUS at LDO in |
| C11 10 µF | 3V3 at LDO out |
| C8 **1 µF** | RP2040 **VREG_OUT** → both **DVDD** (not 3V3) |
| C9 **1 µF** | RP2040 **VREG_IN** / 3V3 |
| C1, C3–C6, C14 100 nF | 3V3 local (MCU / flash / QT / OLED / opto) |
| C2 **1 µF** | IQS572 **VDDHI** → GND |
| C7 **1 µF** | IQS572 **VREG** (internal regulator **output**) → GND only — not tied to 3V3 |

**TESTEN → GND.**

---

## 2. Crystal

**ABM8-272-T3** on XIN/XOUT: C12/C13 15 pF C0G to GND; **R7 1 kΩ** series on XOUT.

---

## 3. QSPI flash

`W25Q32JVSSIQ` (LCSC [C179173](https://www.lcsc.com/product-detail/C179173.html), SOIC-8 208 mil) → QSPI_SS / SCLK / SD0–3. Same pads/pinout as the 16 Mbit parts. IQ = Quad Enable factory-set (needed for RP2040). Default Pico SDK `boot2_w25q080`. Waveshare RP2040-Plus 4 MB ships this exact MPN. Firmware: `PICO_FLASH_SIZE_BYTES=4*1024*1024`. Do not buy leftover 16 Mbit JV (`W25Q16JVSSIQ` / C131025).

| Ref | Value | Net |
|-----|-------|-----|
| **R30** | **10 kΩ** | `~QSPI_SS` → 3V3 (always populated) |
| **R5** | **1 kΩ** | `~QSPI_SS` → SW13 pad 3 |
| **SW13** | TS-1187A | pad 1 → GND; pad 3 → R5. Commons are **top 1+2 / bottom 3+4**. Pads 2 and 4 NC on sch. |

**RUN:** R6 10 kΩ → 3V3. No RUN button.

---

## 4. USB (one port)

**J1** Hroparts `TYPE-C-31-M-12` ([C165948](https://www.lcsc.com/product-detail/C165948.html)) on **B.Cu**, board edge. Per [`lib/USB.md`](lib/USB.md): DP1↔DP2, DN1↔DN2; **USBLC6**; **R3/R4 22 Ω**; CC **R1/R2 5.1 kΩ→GND** (do not tie CC1 to CC2); VBUS→LDO; SBU NC.

---

## 5. MIDI Type A

[`lib/TRS.md`](lib/TRS.md) + [`lib/MIDI.md`](lib/MIDI.md). UART0: **GPIO0 TX / GPIO1 RX**.

**OUT (J2):** Tip ← R12 33 Ω ← 3V3; Ring ← R11 10 Ω ← TX; Sleeve GND.

**IN (J3 + U9 TLP2361):** Tip → R10 220 Ω → AN; Ring → CAT; VO → RX; VCC=3V3 + C14 100 nF; D1 1N4148 reverse across LED.

Place the jacks so the **metal bushing is past Edge.Cuts** (plastic body + pins on FR4). Boss holes are mechanical, no net.

---

## 6. I²C — **I2C1** (not I2C0)

RP2040 mux: **GPIO10 = I2C1 SDA**, **GPIO11 = I2C1 SCL**. Firmware must use **I2C1**.

| Device | Address | Notes |
|--------|---------|-------|
| OLED DS1 | `0x3C` | Module must be **GND / VCC / SCL / SDA** |
| QT2120 U4 | `0x1C` | MODE→GND, RESET→3V3, CHANGE open |
| IQS572 U3 | `0x74` | **RDY → GPIO8** (required); **NRST → GPIO9** |

Pull-ups **R8/R9 4.7 kΩ** to 3V3.

---

## 7. Touch series R

| Ref | Value | Net |
|-----|-------|-----|
| **R13–R15** | **10 kΩ** | QT2120 KEY0–2 ↔ strip E1–E3 |
| **R16–R29** | **1 kΩ** | IQS572 Rx0–6 / Tx0–6 ↔ trackpad silk Rx1–7 / Tx1–7 |

Unused IQS Rx7 / Tx7 / Tx8 and unused QT KEY3–11 left open.

Keepout on internal planes covers **sense** copper (KEY0–2 / trackpad diamonds). Slider **GND end chevrons** (U2 pads 4–5) may sit outside keepout on purpose.

---

## 8. GPIO map (live schematic)

| GPIO | Function |
|------|----------|
| 0 | MIDI OUT UART0 TX |
| 1 | MIDI IN UART0 RX |
| 2 | SW5 Gateron |
| 3 | SW11 EVQ |
| 4 | SW9 EVQ |
| 5 | SW10 EVQ |
| 6 | SW7 Gateron |
| 7 | unused |
| 8 | IQS572 RDY |
| 9 | IQS572 NRST |
| 10 | I2C1 SDA |
| 11 | I2C1 SCL |
| 12 | SW8 Gateron |
| 13 | SW6 Gateron |
| 14–16 | unused |
| 17 | SW4 Gateron |
| 18 | SW2 Gateron |
| 19–23 | unused |
| 24 | SW1 Gateron |
| 25 | SW3 Gateron |
| 26–29 | open (ADC spare / Alps fallback) |

Gateron / EVQ: one pole → GPIO (MCU internal pull-up), other → GND. Pressed = low.
