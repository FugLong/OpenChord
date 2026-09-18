# M1 support circuit — verified

Companion to [`bom.md`](bom.md). Place these **before** wiring nets.

---

## Completeness check (rev A product board)

| Block | Need | Verdict |
|-------|------|---------|
| MCU | RP2040 | Have (on sch) |
| Boot flash | W25Q16JVSSIQ QSPI | **Locked** — place |
| Clock | ABM8-272-T3 + 15 pF ×2 + 1 kΩ | **Locked** — place |
| 3V3 | AP2112K + bulk caps | **Locked** — place |
| USB | USB4105 + USBLC6 + **22 Ω** (on sch) + CC 5.1 kΩ | Connector + series R have; place ESD |
| MIDI OUT | 10 Ω + 33 Ω Type A | Place Rs |
| MIDI IN | **TLP2361** + 220 Ω + 1N4148 + 100 nF | **Locked** — place `oc-midi` |
| Touch | IQS572 + QT2120 + electrodes | Have |
| Display | OLED | Have |
| Keys | 8× Gateron | Have |
| System btns | EVQ-PUA02K ×3 | Have — **GPIO; roles TBD** |
| BOOTSEL | TS-1187A top tact + 1 kΩ | Place — **QSPI_SS strap** (`oc-btn:TS-1187A-B-A-B`) |
| RUN / RST | same tact (optional) | Place if you want a reset button |
| I2C | 4.7 kΩ ×2 | Place |
| RUN | 10 kΩ pull-up | Place |
| RP2040 ties | VREG 1 µF ×2; IO/DVDD 100 nF; **TESTEN→GND**; ADC_AVDD→3V3 | Place |

**Not required for rev A:** second USB-C, Alps stick, B3F (DNP fallbacks only), ferrite on VBUS (optional), SWD header (nice-to-have).

**Sch note:** **3× EVQ** = system buttons → GPIO (roles TBD). **BOOTSEL** / optional **RUN** = top tact `TS-1187A-B-A-B` (not the edge PUA). Do not wire QT2120 SNS to buttons.

---

## KiCad — what to place (stock vs project)

| Part | Symbol | Footprint / 3D |
|------|--------|----------------|
| RP2040 | `MCU_RaspberryPi:RP2040` | stock QFN-56 7×7 (already placed) |
| W25Q16JVSSIQ | `Memory_Flash:W25Q16JVSS` | stock `SOIC-8_5.3x5.3mm_P1.27mm` + 3D |
| ABM8-272-T3 | `Device:Crystal_GND24` value=`ABM8-272-T3` | stock `Crystal_SMD_3225-4Pin_3.2x2.5mm` + 3D |
| AP2112K-3.3TRG1 | `Regulator_Linear:AP2112K-3.3` | stock `SOT-23-5` + 3D |
| USBLC6-2SC6 | `Power_Protection:USBLC6-2SC6` | stock `SOT-23-6` + 3D |
| **TLP2361** | **`oc-midi:TLP2361`** | **project** STEP/WRL |
| 1N4148WS | `Diode:1N4148WS` | stock `D_SOD-323` |
| Passives | `Device:R` / `C` / `C_Small` | stock 0603 / 0805 |
| BOOTSEL / Mode | `oc-btn:EVQ-PUA02K` | project (already) |
| USB / TRS / touch / OLED / Gateron | `oc-*` / `captouch` | project (already) |
| SWD (optional) | `Connector_PinHeader_2.54mm:PinHeader_1x04` | stock |

Reload project libs if KiCad was open when `oc-midi` was added.

---

## 1. Power

```
USB VBUS ──► AP2112K VIN ──► +3V3 ──► RP2040 IOVDD / USB_VDD / VREG_IN / ADC_AVDD
                 EN ← VIN                 flash, OLED, QT2120, IQS572, TLP2361 VCC
```

| Cap | Where |
|-----|--------|
| 10 µF | VBUS at LDO in |
| 10 µF | 3V3 at LDO out |
| 100 nF | near LDO |
| **1 µF** | RP2040 **VREG_IN** (tight) |
| **1 µF** | RP2040 **VREG_OUT** → all **DVDD** (tight) |
| ~8–10× 100 nF | each IOVDD / USB_VDD / DVDD / flash VCC |

**TESTEN → GND.** Do not leave floating.

---

## 2. Crystal

**ABM8-272-T3** on XIN/XOUT: 15 pF C0G each side to GND; **1 kΩ** series on XOUT. Short traces.

---

## 3. QSPI flash

`W25Q16JVSS` → QSPI_SS / SCLK / SD0–3 direct.

**BOOTSEL:** EVQ + **1 kΩ** from QSPI_SS to GND. Optional 10 kΩ pull-up SS→3V3 (pad OK if DNP).

**RUN:** 10 kΩ → 3V3.

---

## 4. USB (one port)

Per [`lib/USB.md`](lib/USB.md): DP/DN shorts; **USBLC6**; **22 Ω** series (R3/R4 already); CC **5.1 kΩ→GND**; VBUS→LDO.

---

## 5. MIDI Type A

[`lib/TRS.md`](lib/TRS.md) + [`lib/MIDI.md`](lib/MIDI.md).

**OUT:** Tip ← 33 Ω ← 3V3; Ring ← 10 Ω ← TX; Sleeve GND.

**IN (TLP2361):** Tip → 220 Ω → AN; Ring → CAT; VO → RX; VCC=3V3 + **100 nF**; 1N4148 across LED.

---

## 6. I2C0

OLED `0x3C` + QT2120 `0x1C` + IQS572 `0x74`. Pull-ups **4.7 kΩ** ×2. **RDY** → GPIO.

---

## 7. Touch series R (**locked**)

| Ref | Value | Net | Why |
|-----|-------|-----|-----|
| R14–R16 | **10 kΩ** | QT2120 ↔ strip E1–E3 | QT2120 datasheet Rs **4.7–20 kΩ** |
| R17–R30 | **1 kΩ** | IQS572 ↔ trackpad **Rx0–6 + Tx0–6** (U5 silk Rx1–7 / Tx1–7) | Azoteq AZD068 / IQS525 ref (100 Ω–1 kΩ) |

**Not capacitive:** 3× system EVQ → **GPIO** (roles TBD). BOOTSEL → **QSPI_SS** (see below), not a user GPIO. Gaterons → GPIO.

---

## Place checklist

- [ ] LDO + power symbols + caps  
- [ ] RP2040 VREG + decoupling + **TESTEN→GND** + ADC_AVDD→3V3  
- [ ] Crystal + 15 pF ×2 + 1 kΩ  
- [ ] Flash + 100 nF + BOOTSEL EVQ + 1 kΩ  
- [ ] USBLC6 (series R + CC already on sch)  
- [ ] **oc-midi:TLP2361** + 220 Ω + 1N4148 + 100 nF; MIDI OUT Rs  
- [ ] I2C 4.7 kΩ ×2; RUN 10 kΩ  
- [ ] Touch VREG caps + **series R R14–R30** (10 kΩ strip / 1 kΩ trackpad)  
- [ ] Gaterons + 3× system EVQs → **RP2040 GPIO** (roles TBD; no touch series R)  
- [ ] BOOTSEL EVQ on **QSPI_SS** (+ 1 kΩ) — UF2 only  
- [ ] **Then** wire nets
