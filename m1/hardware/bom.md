# M1 BOM (rev A)

Matches `OpenChordM1/OpenChordM1.kicad_sch`. Wiring: [`support.md`](support.md). Touch: [`touch.md`](touch.md).

**Hand-build.** 0603 unless noted. Electrodes **U2 / U5** and test pads are copper — not buy items.

Prices from carts **2026-09-19**: [LCSC](https://www.lcsc.com/) `$40.78` + [DigiKey](https://www.digikey.com/) `$29.50` = **`$70.28` for 5**. Every schematic electronic is in one of those two carts. Still buy OLED / Gateron / PCB / cover elsewhere.

## Cost

| | **1 unit** | **5 units** |
|--|----------:|----------:|
| LCSC + DigiKey carts (MOQ leftovers in) | 14.06 | **70.28** |
| OLED | 1.20 | 6.00 |
| 8× socket + 8× KS-33 | 3.04 | 15.20 |
| 4-layer PCB + DHL | ~10 | ~50 |
| Printed cover | ~2 | ~10 |
| Cart shipping | ~4 | ~20 |
| **Landed (US, no labor)** | **~$34** | **~$171** |

Not included: assembly, stencil, solder, keycaps, scrap, tariffs. **ENIG** for gold touch pads is extra **~$5–8**/board. Qty ~100 is roughly **$16–20**/unit.

Biggest lines: PCB, IQS **$2.07**, QT **$1.93**, 2× TRS **$1.90**, flash **$1.49**, OLED **$1.20**, RP2040 **$1.00**.

---

## ICs

| Qty | Ref | Part | MPN | Pkg | $ | Buy |
|----:|-----|------|-----|-----|--:|-----|
| 1 | U1 | MCU | RP2040 | QFN-56 7×7 | 1.00 | [LCSC C2040](https://www.lcsc.com/product-detail/C2040.html) |
| 1 | U6 | Flash 32 Mbit | W25Q32JVSSIQ | SOIC-8 208 mil | 1.49 | [LCSC C179173](https://www.lcsc.com/product-detail/C179173.html) |
| 1 | Y1 | 12 MHz | ABM8-272-T3 | 3.2×2.5 | 0.63 | [LCSC C20625731](https://www.lcsc.com/product-detail/C20625731.html) |
| 1 | U7 | 3.3 V LDO | AP2112K-3.3TRG1 | SOT-23-5 | 0.17 | [LCSC C51118](https://www.lcsc.com/product-detail/C51118.html) |
| 1 | U8 | USB ESD | USBLC6-2SC6 | SOT-23-6 | 0.18 | [LCSC C7519](https://www.lcsc.com/product-detail/C7519.html) |
| 1 | U3 | Trackpad | IQS572BLQNR | QFN-28 4×4 | 2.07 | [DigiKey](https://www.digikey.com/en/products/detail/azoteq-pty-ltd/IQS572BLQNR/7165004) (KiCad `IQS572-BL-QNR`) |
| 1 | U4 | Cap strip | AT42QT2120-XUR | TSSOP-20 | 1.93 | [DigiKey](https://www.digikey.com/en/products/detail/microchip-technology/AT42QT2120-XUR/3678735) |
| 1 | U9 | MIDI IN opto | TLP2361(TPL,E) | SO6 | 0.53 | [LCSC C107626](https://www.lcsc.com/product-detail/C107626.html) |
| 1 | D1 | MIDI IN clamp | 1N4148WS | SOD-323 | 0.02 | [LCSC C2128](https://www.lcsc.com/product-detail/C2128.html) |
| | | | | | **8.02** | |

Do not buy leftover **W25Q16JV** / C131025. Flash cart qty is **10** (MOQ).

## Connectors & buttons

| Qty | Ref | Part | MPN | Pkg | $ | Buy |
|----:|-----|------|-----|-----|--:|-----|
| 1 | J1 | USB-C | TYPE-C-31-M-12 | SMT+TH | 0.17 | [LCSC C165948](https://www.lcsc.com/product-detail/C165948.html) |
| 2 | J2 OUT, J3 IN | TRS 3.5 mm | SJ1-3523N | TH R/A | 1.90 | [DigiKey](https://www.digikey.com/en/products/detail/same-sky-formerly-cui-devices/SJ1-3523N/738689) |
| 3 | SW9–SW11 | System | EVQPUC02K | SMD side + bosses | 0.53 | [LCSC C79174](https://www.lcsc.com/product-detail/C79174.html) |
| 1 | SW13 | BOOTSEL | TS-1187A-B-A-B | SMD 5.1×5.1 | 0.02 | [LCSC C318884](https://www.lcsc.com/product-detail/C318884.html) |
| | | | | | **2.62** | |

No RUN button on rev A.

## Display, switches, sockets

Not in the LCSC/DigiKey carts.

| Qty | Ref | Part | $ | Buy |
|----:|-----|------|--:|-----|
| 1 | DS1 | OLED 0.91" white SSD1306 4-pin | 1.20 | [AliExpress](https://www.aliexpress.com/w/wholesale-0.91-oled-ssd1306-128x32-white.html) |
| 8 | SW1–SW8 | LP hotswap socket (KS-27 / KS-33) | 1.25 | [Gateron](https://www.gateron.com/products/gateron-low-profile-switch-hot-swap-pcb-socket) · [beekeeb](https://shop.beekeeb.com/products/gateron-low-profile-hotswap-socket-compatible-with-gateron-ks-27-switches-5pcs) |
| 8 | — | KS-33 (KS-27 OK) | 1.79 | [Gateron 35-pack](https://www.gateron.com/products/gateron-ks-33-low-profile-switch-set) |
| | | | **4.24** | |

Sockets are on the schematic; switches plug in after fab. Five boards need **40** sockets and **40** switches (two 35-packs). OLED pinout GND / VCC / SCL / SDA — [`lib/OLED.md`](lib/OLED.md).

## Passives

In the LCSC cart. Equivalents OK if value + package match. Crystal load caps stay **C0G**. 10 µF is **0805**; everything else **0603**.

| /board | Cart | Value | Refs | Use | $ | LCSC |
|-------:|-----:|-------|------|-----|--:|------|
| 2 | 100 | 22 Ω | R3, R4 | USB D+/D− | 0.00 | [C23345](https://www.lcsc.com/product-detail/C23345.html) |
| 2 | 100 | 5.1 kΩ | R1, R2 | USB-C CC → GND | 0.00 | [C23186](https://www.lcsc.com/product-detail/C23186.html) |
| 16 | 200 | 1 kΩ | R5, R7, R16–R29 | BOOTSEL, XOUT, IQS Tx/Rx | 0.05 | [C21190](https://www.lcsc.com/product-detail/C21190.html) |
| 5 | 100 | 10 kΩ | R6, R13–R15, R30 | RUN PU, QT SNS, QSPI_SS PU | 0.02 | [C98220](https://www.lcsc.com/product-detail/C98220.html) |
| 2 | 100 | 4.7 kΩ | R8, R9 | I2C1 pull-ups | 0.01 | [C23162](https://www.lcsc.com/product-detail/C23162.html) |
| 1 | 100 | 220 Ω | R10 | MIDI IN LED | 0.01 | [C22962](https://www.lcsc.com/product-detail/C22962.html) |
| 1 | 100 | 10 Ω | R11 | MIDI OUT TX → Ring | 0.00 | [C22859](https://www.lcsc.com/product-detail/C22859.html) |
| 1 | 100 | 33 Ω | R12 | MIDI OUT 3V3 → Tip | 0.00 | [C23140](https://www.lcsc.com/product-detail/C23140.html) |
| 6 | 100 | 100 nF | C1, C3–C6, C14 | decoupling | 0.08 | [C14663](https://www.lcsc.com/product-detail/C14663.html) |
| 4 | 100 | 1 µF | C2, C7–C9 | IQS VDDHI/VREG, RP2040 VREG | 0.06 | [C14664](https://www.lcsc.com/product-detail/C14664.html) |
| 2 | 50 | 15 pF C0G | C12, C13 | crystal load | 0.02 | [C1644](https://www.lcsc.com/product-detail/C1644.html) |
| 2 | 40 | 10 µF 0805 | C10, C11 | LDO in / out | 0.13 | [C1713](https://www.lcsc.com/product-detail/C1713.html) |
| | | | | **Consumed / board** | **0.38** | |
| | | | | **Bags / board** (cart ÷ 5) | **1.72** | |

## PCB + cover

**118.95 × 63.15 mm**, 4-layer, **HASL** (KiCad job file may still say ENIG — pick HASL on the order form). [JLCPCB](https://jlcpcb.com/) — **~$10**/board at qty 5. Cover **~$2** filament.

Gerbers: [`fab/jlcpcb/`](fab/jlcpcb/) / zip `fab/OpenChordM1-gerbers.zip`. Stencil: **bottom only**, frameless, **160 × 120 mm**.

---

## Orders (5 boards)

Re-import these CSVs (vendor export format, quantities for 5 boards):

- LCSC: [`carts/OpenChordM1-LCSC-qty5.csv`](carts/OpenChordM1-LCSC-qty5.csv)
- DigiKey: [`carts/OpenChordM1-DigiKey-qty5.csv`](carts/OpenChordM1-DigiKey-qty5.csv)

1. **LCSC `$40.78`** — 22 lines: C2040, C179173 (qty 10), C165948, C51118, C7519, C20625731, C107626, C2128, C79174 (qty 15), C318884 (qty 20), plus the 12 passives above.
2. **DigiKey `$29.50`** — [IQS572BLQNR](https://www.digikey.com/en/products/detail/azoteq-pty-ltd/IQS572BLQNR/7165004) ×5, [AT42QT2120-XUR](https://www.digikey.com/en/products/detail/microchip-technology/AT42QT2120-XUR/3678735) ×5, [SJ1-3523N](https://www.digikey.com/en/products/detail/same-sky-formerly-cui-devices/SJ1-3523N/738689) ×10.
3. 5× OLED; **40** Gateron LP sockets + **40** KS-33 (two 35-packs).
4. JLCPCB 4-layer HASL, ~119 × 63 mm, qty 5, tented vias. Upload `fab/OpenChordM1-gerbers.zip`. Bottom stencil 160 × 120 mm frameless.
