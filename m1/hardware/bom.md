# M1 BOM (rev A)

Locks: [`../docs/goals.md`](../docs/goals.md). Wiring recipe: [`support.md`](support.md). Touch: [`touch.md`](touch.md).

**Hand-build only.** Bare PCB. Hot air: RP2040 + **IQS572** + **QT2120**. Iron: the rest. Packages **0603** unless noted.

**Cost (small batch ~10–25):** parts + PCB + printed cover ≈ **$25–35**/unit (switches included). Street under **$100**.

---

## ICs / connectors (qty = 1 board)

Order by **MPN** (DigiKey search) or **LCSC #**. No fragile DigiKey product IDs.

| Qty | What | MPN | LCSC | Pkg | Status |
|-----|------|-----|------|-----|--------|
| 1 | MCU | **RP2040** | **C2040** | QFN-56 7×7 | **locked** |
| 1 | Flash 16 Mbit | **W25Q16JVSSIQ** | **C131025** | SOIC-8 | **locked** — Winbond QSPI; 2 MB |
| 1 | 12 MHz crystal | **ABM8-272-T3** | **C20625731** | 3.2×2.5 | **locked** — Pico / RPi pick |
| 1 | 3.3 V LDO | **AP2112K-3.3TRG1** | **C51118** | SOT-23-5 | **locked** — 600 mA |
| 1 | USB-C | **USB4105-GF-A** | **C3020560** | SMT+TH | **locked** — `oc-usb`; one port |
| 1 | USB ESD | **USBLC6-2SC6** | **C7519** | SOT-23-6 | **locked** |
| 1 | Cap trackpad | **IQS572BLQNR** | **C3827635** | QFN-28 4×4 0.5 | **locked** — `oc-touch` |
| 1 | Cap strip (slider) | **AT42QT2120-XUR** | **C1522278** | TSSOP-20 | **locked** — `oc-touch`; strip only |
| 1 | MIDI IN opto | **TLP2361(TPL,E** | **C107626** | SO6 | **locked** — 2.7–5.5 V totem-pole; **not** H11L1 |
| 1 | MIDI IN clamp | **1N4148WS** | **C2128** | SOD-323 | **locked** — across opto LED |
| 2 | TRS 3.5 mm | **SJ1-3523N** | **C20182914** | TH R/A | **locked** — `oc-trs`; Type A |
| 3 | System btn | **EVQ-PUA02K** | **C128539** | SMD side | **locked** part — **roles TBD** (not capacitive) |
| 1 | BOOTSEL | **TS-1187A-B-A-B** | **C318884** | SMD top 5.1×5.1 | **locked** — UF2; under cover; `oc-btn` |
| 0–1 | RUN / RST | **TS-1187A-B-A-B** | **C318884** | same | optional — same part |
| 1 | OLED 0.91" white | Ali SSD1306 4-pin | — | TH | **have** |
| 8 | LP hotswap | Gateron LP socket | — | bottom SMD | ordered |
| 8 | LP switch | KS-27 / KS-33 | — | 2-pin | **have** |

### Fallback (DNP on product sch)

| Qty | What | MPN | LCSC | When |
|-----|------|-----|------|------|
| 1 | Stick | RKJXV1220001 | **C219778** | Trackpad feel fails |
| 2–3 | TH tactile | B3F-1000 | — | Extra EVQ / bring-up only |

Cut: SoftPot, matrix diodes, H11L1 / 6N138 as MIDI IN, second QT2120, PTC, FPC OLED, paid PCBA. Second USB-C optional later — not on rev A.

---

## Passives (0603; DigiKey/LCSC generic OK — match value)

Exact LCSC #s are JLCPCB-friendly; any 1% / C0G equivalent works for hand-build.

| Qty | Value | Role | Notes |
|----:|-------|------|-------|
| 2 | 22 Ω | USB D+/D− series | Already on sch (R3/R4); 22 Ω is fine |
| 2 | 5.1 kΩ | USB-C CC1/CC2 → GND | UFP |
| 1 | 1 kΩ | QSPI_SS ↔ BOOTSEL (to GND via btn) | Per RPi design |
| 1 | 10 kΩ | QSPI_SS pull-up → 3V3 | Optional DNP if flash holds CS; place pad |
| 1 | 1 kΩ | Crystal series (XOUT) | RPi crystal circuit |
| 2 | 15 pF C0G | Crystal load | ABM8-272-T3 CL=10 pF |
| 2 | 10 kΩ | RUN pull-up; spare | RUN → 3V3 |
| 2 | 4.7 kΩ | I2C0 SDA/SCL pull-ups | OLED + QT2120 + IQS572 |
| 1 | 220 Ω | MIDI IN series (LED drive) | Tip → anode (Type A) |
| 1 | 10 Ω | MIDI OUT TX → Ring | Type A |
| 1 | 33 Ω | MIDI OUT 3V3 → Tip | Type A (~5 mA) |
| ~12 | 100 nF | RP2040 + flash + USB + opto + LDO | See [`support.md`](support.md) |
| 2 | 1 µF | RP2040 VREG_IN / VREG_OUT | Mandatory, close to chip |
| 2 | 10 µF 0805 | LDO in (VBUS) + LDO out (3V3 bulk) | |
| 2–4 | 100 nF + 1 µF | IQS572 (VDDHI / VREG) | Per Azoteq |
| 1–2 | 100 nF | QT2120 Vdd | Per Microchip |
| 3 | 10 kΩ | QT2120 SNS series (strip E1–E3) | Datasheet Rs **4.7–20 kΩ**; lock **10 kΩ** |
| 14 | 1 kΩ | IQS572 Tx/Rx series (trackpad 7×7) | U5 → chip **Rx0–6 / Tx0–6** (leave Rx7, Tx7, Tx8 open) |

---

## Eval / bring-up (not product)

| What | Why | Link |
|------|-----|------|
| **TPS43-201A-S** | IQS572 module → Zero | [DigiKey](https://www.digikey.com/en/products/detail/azoteq-pty-ltd/TPS43-201A-S/7164940) |
| IQS572EV02 | Official trackpad + GUI | [Azoteq](https://www.azoteq.com/product/iqs572-b000/) |
| Touchy Subject | QT2120 breakout | [Lectronz](https://www.lectronz.com/products/touchy-subject) |

---

## Buy order

1. DigiKey: RP2040, **W25Q16JVSSIQ**, **ABM8-272-T3**, **AP2112K**, **USBLC6**, **TLP2361**, **IQS572**, **QT2120-XUR**, USB4105, SJ1-3523N, EVQ-PUA ×2, passives kit.
2. Or LCSC for the same MPNs (CAD already matches EasyEDA imports).
3. Coupons / TPS43 before product fab — [`touch.md`](touch.md).
