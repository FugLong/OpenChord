# OpenChord M1 — Hardware verification handoff

**Purpose:** Give a fresh agent (or human) enough context to re-verify schematic + PCB.  
**Date context:** 2026-09-19, KiCad 10.0.6, rev A **ordered** (4-layer HASL, qty 5).  
**Owner intent:** Finger-on-PCB capacitive UI (soldermask over electrodes, tented sense vias), 4-layer, UI-dense top / everything else bottom.

**2026-09-19 pre-order:** DRC **0 shorts, 0 unconnected, 0 schematic_parity**. USB-C J1 and TRS J2/J3 moved; bushings off-board. Gerbers in `fab/jlcpcb/`. Remaining DRC is USB-C/IQS pad pitch, silk-on-edge, OLED courtyard vs jack PTH, starved thermals (1 spoke). Not blockers.

---

## 1. Product / electrical intent (locked)

| Block | Intent |
|-------|--------|
| MCU | RP2040 + W25Q32JVSSIQ ([C179173](https://www.lcsc.com/product-detail/C179173.html)) QSPI + 12 MHz crystal |
| Power | USB-C VBUS → AP2112K-3.3 → `+3.3V` (EN tied to VIN) |
| USB | J1 TYPE-C-31-M-12 (C165948) + USBLC6 + 22Ω series + CC 5.1k→GND |
| Display | 0.91" SSD1306 OLED I²C `0x3C` |
| Trackpad | IQS572 + 7×7 diamond matrix, I²C `0x74`, **RDY required** |
| Slider | QT2120 **strip only** KEY0–2, I²C `0x1C`, MODE→GND, RESET→3V3 |
| Keys | 8× Gateron LP → GPIO (internal pull-ups) |
| System | 3× EVQPUC02K side → GPIO (roles TBD) |
| BOOTSEL | Strap `~QSPI_SS` through **R5 1 kΩ** to GND via TS-1187A (not a user GPIO); **R30 10 kΩ** SS→3V3 (populated) |
| MIDI | Type A TRS: OUT tip=3V3 via 33Ω, ring=TX via 10Ω; IN tip→220Ω→TLP2361 AN, ring→CAT, VO→RX |
| I²C | **I2C1** GPIO10 SDA / GPIO11 SCL, 4.7 kΩ×2; OLED `0x3C` + QT `0x1C` + IQS `0x74`; IQS RDY GPIO8, NRST GPIO9 |
| Updates | After first flash, firmware can `reset_usb_boot()` for buttonless UF2; BOOTSEL kept for recovery |

**Docs match the rev A schematic** (`support.md` is the as-built pin/passive map). If they ever disagree, prefer **live schematic/netlist**.

**Paths:**
- Sch: `m1/hardware/OpenChordM1/OpenChordM1.kicad_sch`
- PCB: `m1/hardware/OpenChordM1/OpenChordM1.kicad_pcb`
- Libs: `m1/hardware/lib/` (`oc-*`, `captouch`, `easyeda/`)

---

## 2. How to re-verify (procedure)

1. Export schematic netlist:
   ```bash
   kicad-cli sch export netlist --format kicadsexpr \
     -o /tmp/oc_sch.sexpr m1/hardware/OpenChordM1/OpenChordM1.kicad_sch
   ```
2. Run ERC + DRC with schematic parity:
   ```bash
   kicad-cli sch erc -o /tmp/oc_erc.json --format json --severity-all ...
   kicad-cli pcb drc -o /tmp/oc_drc.json --format json --schematic-parity --severity-all ...
   ```
3. Walk **every active** in §4 against datasheet pin tables (not just “nets look connected”).
4. Confirm RP2040 mux: I²C / UART pins must match pico-sdk function table.
5. Confirm switch **poles** against the **datasheet**, not against “pads on the same side of the footprint.” For TS-1187A, commons are **top 1+2 / bottom 3+4**.
6. Spot-check PCB: ratsnest clear, zones refill, keepouts under trackpad+slider on internal planes, sense series R near chips.

**Known noisy ERC/DRC:** EasyEDA “Unspecified” pin types → `pin_to_pin` spam; captouch via/pad annular quirks; courtyard/silk on dense UI. User believes remaining are false positives or intentional — **re-check, don’t blindly trust.**

---

## 3. Snapshot findings (2026-09-19 pre-order)

### Schematic ↔ PCB
- DRC **schematic_parity: 0 issues** at last audit.
- **C5 GND** open and a **GND zone island** were found then **fixed** (trace to via; pour connectivity).
- **GPIO3** was briefly shorted to **+3.3V** by a via after R30 at ~(133.90, 54.18) — **fixed** by moving the via to ~(134.50, 53.58) (~0.64 mm clearance). Re-check DRC shorts = 0 before fab.

### Critical / cleared

#### C1 — BOOTSEL SW13 poles — **CLEARED (was a false alarm)**
- Part: `TS-1187A-B-A-B`. XKB datasheet + EasyEDA symbol: **top 1+2** one pole, **bottom 3+4** other; press bridges top↔bottom.
- Footprint geometry is left/right columns, but **electrical commons are top/bottom rows**, not left/right.
- Netlist: **pad1=GND** (top), **pad3→R5→`~QSPI_SS`** (bottom) = opposite poles → **correct**.
- Optional hygiene: also tie pad2 to GND and pad4 to the R5 net (both pads of each pole).
- **Do not** re-open this as “1+3 same pole” — that was an agent mistake from assuming side-commons.

#### C2 — (Cleared) QT2120 pin numbers
- Earlier false alarm compared **VQFN** pinout to **TSSOP** symbol.
- Part is **AT42QT2120-XUR** + footprint `TSSOP-20_...`.
- Microchip **TSSOP/SOIC** table matches EasyEDA symbol: 11=VDD, 12=MODE, 13=SDA, 14=RESET, 16=SCL.
- Still verify PCB pin-1 marker / rotation against TSSOP package drawing before fab.

### High / medium follow-ups
| ID | Item | Notes |
|----|------|-------|
| H1 | IQS572 **NRST→GPIO9** | Chip has **internal NRST pull-up**; OK if GPIO floats briefly. FW must release/drive high early. Optional: external PU to 3V3. |
| H2 | IQS572 **RDY→GPIO8** | Required for reliable I²C on shared bus — confirm still wired. |
| M1 | QT **CHANGE** open | OK if polling I²C. |
| M2 | QT **RESET→+3.3V** | Correct per datasheet when not MCU-driven. |
| M3 | I2C1 (not I2C0) | **Docs updated.** GPIO10/11 = I2C1 mux — firmware must use I2C1. |
| M4 | Series R refdes | **Docs updated.** Strip **R13–R15 10 kΩ**; trackpad **R16–R29 1 kΩ**; **R30** is QSPI_SS pull-up. |
| M5 | No RUN button | Optional; RUN has R6 10 kΩ to 3V3. |
| M6 | GPIO7,14–16,19–23 unused | Fine spare. |
| M7 | IQS **C2 = 1 µF** on VDDHI; **C7 = 1 µF** on VREG (cap to GND only). Optional 100 pF across C7 skipped. |

### Blocks that looked correct at audit
| Block | Result |
|-------|--------|
| RP2040 power (IOVDD, VIN, VOUT↔DVDD×2, USB_VDD, ADC_AVDD, TESTEN→GND, RUN↑) | PASS |
| Crystal + 15pF + 1k on XOUT | PASS |
| QSPI flash map (incl. pad54=SD2→WP, pad55=SD1→DO) | PASS |
| USB ESD + 22Ω + CC + LDO | PASS |
| I²C bus + 4.7k pull-ups on GPIO10/11 | PASS (I2C1) |
| MIDI OUT Type A + GPIO0 UART0 TX | PASS |
| MIDI IN opto + GPIO1 UART0 RX | PASS |
| 8× Gateron + 3× EVQ to GPIOs | PASS |
| SWD / VBUS / GND testpoints | PASS |
| IQS Rx0–6/Tx0–6 + 1k; QT KEY0–2 + 10k; MODE→GND | PASS |

### GPIO map (audit snapshot — re-extract if sch changed)

| GPIO | Function |
|------|----------|
| 0 | MIDI OUT (UART0 TX) |
| 1 | MIDI IN (UART0 RX) |
| 2 | SW5 Gateron |
| 3 | SW11 EVQ |
| 4 | SW9 EVQ |
| 5 | SW10 EVQ |
| 6 | SW7 Gateron |
| 7 | unused |
| 8 | IQS RDY |
| 9 | IQS NRST |
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
| 26–29 | open (ADC-capable spare / Alps fallback) |

**Mux cheat-sheet:** GPIO10/11 = I2C1 SDA/SCL ✓ · GPIO0/1 = UART0 TX/RX ✓ · GPIO8/9 = plain GPIO (RDY/NRST) ✓

---

## 4. Active-by-active datasheet checklist

Use this as the slow pass. For each: open MPN datasheet, confirm every pin.

### U1 RP2040
- [ ] All IOVDD → 3V3 (stacked symbol OK; all pads must hit pour)
- [ ] VREG_VIN → 3V3 + 1µF
- [ ] VREG_VOUT ↔ both DVDD + 1µF (**not** 3V3)
- [ ] USB_VDD, ADC_AVDD → 3V3
- [ ] TESTEN → GND
- [ ] RUN ↑ 10k to 3V3
- [ ] Flash QSPI wiring per datasheet pad numbers
- [ ] USB DM/DP through series R
- [ ] Assigned UART/I2C pins exist in mux table

### U6 W25Q32JVSSIQ (LCSC [C179173](https://www.lcsc.com/product-detail/C179173.html))
- [ ] CS/CLK/DI/DO/WP/HOLD ↔ SS/SCLK/SD0/SD1/SD2/SD3
- [ ] VCC 3V3, GND; local decoupling nearby
- [ ] SOIC-8 208 mil (`Package_SO:SOIC-8_5.3x5.3mm_P1.27mm`); buy **C179173**

### Y1 ABM8-272-T3
- [ ] Load caps to GND; series R on XOUT path as designed

### U7 AP2112K-3.3
- [ ] VIN=VBUS, EN=VIN or high, VOUT=3V3, bulk in/out

### U8 USBLC6-2SC6
- [ ] Connector-side D+/D− to I/Os; chip-side through 22Ω to MCU; VBUS pin to VBUS

### J1 TYPE-C-31-M-12 (C165948)
- [ ] VBUS **A4B9** + **B4A9**; GND **A1B12** + **B1A12** + shell **EH 1–4**
- [ ] CC1 **A5** and CC2 **B5** each 5.1k→GND (not tied to each other)
- [ ] DP1 **A6** ↔ DP2 **B6**; DN1 **A7** ↔ DN2 **B7**; SBU A8/B8 NC

### U9 TLP2361 + D1 + MIDI jacks
- [ ] IN: Tip→220→AN, Ring→CAT, diode reverse across LED, VO→RX, VCC+100nF
- [ ] OUT Type A: Tip←33←3V3, Ring←10←TX, Sleeve GND

### U3 IQS572
- [ ] VDDHI 3V3 + **1 µF** (C2); VSS/EP GND; VREG **1 µF** to GND only (C7)
- [ ] SDA/SCL on bus; **RDY→GPIO**; NRST OK open or GPIO with internal PU
- [ ] Rx0–6 / Tx0–6 only (unused open); series 1k each
- [ ] PGM/SW_IN open OK

### U4 QT2120-XUR (TSSOP)
- [ ] Use **TSSOP** pin table (not VQFN)
- [ ] MODE→GND (I²C), RESET→VDD, SDA/SCL on bus
- [ ] KEY0–2 → strip via 10k; other KEY open
- [ ] CHANGE optional

### DS1 OLED
- [ ] GND/VCC/SCL/SDA order matches module (`lib/OLED.md`)

### SW1–8 Gateron / SW9–11 EVQ / SW13 BOOT
- [ ] Keys: GPIO↔GND with MCU pull-ups
- [ ] BOOT: **opposite poles**, R5 1 kΩ on SS side, **R30 10 kΩ** SS→3V3

---

## 5. Layout / fab notes (non-netlist)

- Keepouts under **sense** copper (trackpad diamonds + slider KEY0–2) on **internal** GND/PWR planes. Slider **GND end chevrons** (U2 pads 4–5) may sit **outside** keepout so parts can share that area — intentional.
- Backside parts under trackpad: series Rs OK; avoid noisy MCU under trackpad matrix if possible; under slider is riskier but may be necessary.
- Captouch thru-hole vias: tent by removing `*.Mask` from via pads in footprint (user may have done this).
- Bare-finger soldermask-over-copper is intentional; rev A finish is **HASL** (not ENIG).
- Optional later: ENIG + open mask on electrodes if gold feel is needed.
- 4-layer `Sig–GND–PWR–Sig` (In1 GND, In2 3.3 V). SMT on B.Cu.
- TRS: hang **SJ1-3523N** bushings off Edge.Cuts; do not sit the barrel on FR4.
- Gerbers: `m1/hardware/fab/jlcpcb/`. Stencil: bottom, frameless, 160 × 120 mm.

---

## 6. Firmware assumptions to document for software

- I²C peripheral: **I2C1** on GPIO10/11 (not I2C0).
- UART0 MIDI on GPIO0 TX / GPIO1 RX @ 31250.
- Drive/release IQS NRST (GPIO9) high after boot; poll/wait IQS RDY (GPIO8).
- QT: enable slider on channels 0–2; RESET is hard-tied high; CHANGE unconnected (poll I²C).
- Implement `reset_usb_boot()` for web UF2 updates; keep physical BOOTSEL for recovery.
- **R30** is populated 10 kΩ on `~QSPI_SS`. No RUN button.
- Flash is **4 MB** (`W25Q32JVSSIQ`, LCSC [C179173](https://www.lcsc.com/product-detail/C179173.html)); set `PICO_FLASH_SIZE_BYTES=4*1024*1024`. Default `boot2_w25q080`.

---

## 7. Fresh-agent prompt (copy/paste)

```text
Read m1/hardware/VERIFY_HANDOFF.md fully. Re-export the live schematic netlist and
PCB DRC with --schematic-parity. Independently verify every checklist item in §4.
Note: C1 BOOTSEL poles were a false alarm — datasheet is top 1+2 / bottom 3+4.
Do not revive the left/right commons claim. Produce a short PASS/FAIL table;
only edit hardware if the user asks.
```

---

## 8. Pass criteria for “fab OK”

- [ ] BOOTSEL continuity: SS↔GND **only while pressed**, through 1k (poles top/bottom)
- [ ] QT footprint orientation / pin-1 verified against TSSOP drawing
- [ ] RDY still on a GPIO; I2C1 pins still valid pair
- [ ] MIDI UART pins still valid TX/RX pair
- [ ] Schematic parity clean; DRC shorts = 0; any leftover unconnected/GND items understood
- [ ] Zone keepouts present under capacitive electrodes
- [ ] Firmware pin map matches §GPIO table (or table updated)
