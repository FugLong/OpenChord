# M1 BOM (rev A)

Matches `OpenChordM1/OpenChordM1.kicad_sch`. Wiring: [`support.md`](support.md). Touch: [`touch.md`](touch.md).

**Hand-build.** Packages **0603** unless noted. Electrodes (U2/U5) and test pads are PCB copper — not buy items.

**Buy LCSC** unless the Buy column says DigiKey (OOS or LCSC unit $ is an outlier). Passives: LCSC **100+**. USD list at qty ~10, 2026-09-18, no tax. Links go to the product page you should order from.

## Cost (qty ~10)

| Bucket | $/board |
|--------|--------:|
| Electronics (ICs, connectors, OLED, passives) | **11.09** |
| 8× LP switch + 8× hotswap socket | **3.04** |
| PCB, 4-layer 118.95 × 63.15 mm | **~6.50** |
| Printed cover | **~2.00** |
| **COGS** | **~$22.60** |
| + distributor shipping | **+$2–4** |
| **Landed (US, no labor)** | **~$26–28** |

Not included: assembly time, stencil, solder, keycaps, tariffs, scrap. Qty ~100 is roughly **$16–20**.

Biggest lines: PCB ~$6.50, 2× TRS ~$1.63, IQS572 $1.50, QT2120 $1.50, flash $1.49, OLED ~$1.20, RP2040 $0.91.

---

## ICs

| Qty | Ref | Part | MPN | LCSC | Pkg | $ | Ext | Buy |
|----:|-----|------|-----|------|-----|--:|----:|-----|
| 1 | U1 | MCU | RP2040 | [C2040](https://www.lcsc.com/product-detail/C2040.html) | QFN-56 7×7 | 0.91 | 0.91 | [LCSC](https://www.lcsc.com/product-detail/C2040.html) |
| 1 | U6 | Flash 32 Mbit | W25Q32JVSSIQ | [C179173](https://www.lcsc.com/product-detail/C179173.html) | SOIC-8 208mil | 1.49 | 1.49 | [LCSC](https://www.lcsc.com/product-detail/C179173.html) |
| 1 | Y1 | 12 MHz crystal | ABM8-272-T3 | [C20625731](https://www.lcsc.com/product-detail/C20625731.html) | 3.2×2.5 | 0.30 | 0.30 | [LCSC](https://www.lcsc.com/product-detail/C20625731.html) |
| 1 | U7 | 3.3 V LDO | AP2112K-3.3TRG1 | [C51118](https://www.lcsc.com/product-detail/C51118.html) | SOT-23-5 | 0.17 | 0.17 | [LCSC](https://www.lcsc.com/product-detail/C51118.html) |
| 1 | U8 | USB ESD | USBLC6-2SC6 | [C7519](https://www.lcsc.com/product-detail/C7519.html) | SOT-23-6 | 0.18 | 0.18 | [LCSC](https://www.lcsc.com/product-detail/C7519.html) |
| 1 | U3 | Trackpad | IQS572BLQNR | [C3827635](https://www.lcsc.com/product-detail/C3827635.html) | QFN-28 4×4 | 1.50 | 1.50 | [DigiKey](https://www.digikey.com/en/products/detail/azoteq-pty-ltd/IQS572BLQNR/7165004) (LCSC $4.17) |
| 1 | U4 | Cap strip | AT42QT2120-XUR | [C1522278](https://www.lcsc.com/product-detail/C1522278.html) | TSSOP-20 | 1.50 | 1.50 | [DigiKey](https://www.digikey.com/en/products/detail/microchip-technology/AT42QT2120-XUR/3678735) (LCSC OOS) |
| 1 | U9 | MIDI IN opto | TLP2361(TPL,E) | [C107626](https://www.lcsc.com/product-detail/C107626.html) | SO6 | 0.41 | 0.41 | [LCSC](https://www.lcsc.com/product-detail/C107626.html) |
| 1 | D1 | MIDI IN clamp | 1N4148WS | [C2128](https://www.lcsc.com/product-detail/C2128.html) | SOD-323 | 0.02 | 0.02 | [LCSC](https://www.lcsc.com/product-detail/C2128.html) |
| | | | | | | | **6.48** | |

## Connectors & buttons

| Qty | Ref | Part | MPN | LCSC | Pkg | $ | Ext | Buy |
|----:|-----|------|-----|------|-----|--:|----:|-----|
| 1 | J1 | USB-C | USB4105-GF-A | [C3020560](https://www.lcsc.com/product-detail/C3020560.html) | SMT+TH | 0.69 | 0.69 | [DigiKey](https://www.digikey.com/en/products/detail/gct/USB4105-GF-A/11198441) (LCSC OOS) |
| 2 | J2, J3 | TRS 3.5 mm (OUT / IN) | SJ1-3523N | [C20182914](https://www.lcsc.com/product-detail/C20182914.html) | TH R/A | 0.82 | 1.63 | [DigiKey](https://www.digikey.com/en/products/detail/same-sky-formerly-cui-devices/SJ1-3523N/738689) (LCSC thin) |
| 3 | SW9–SW11 | System btn | EVQPUC02K | [C79174](https://www.lcsc.com/product-detail/C79174.html) | SMD side + bosses | 0.18 | 0.53 | [LCSC](https://www.lcsc.com/product-detail/C79174.html) |
| 1 | SW13 | BOOTSEL | TS-1187A-B-A-B | [C318884](https://www.lcsc.com/product-detail/C318884.html) | SMD top 5.1×5.1 | 0.02 | 0.02 | [LCSC](https://www.lcsc.com/product-detail/C318884.html) |
| | | | | | | | **2.87** | |

No RUN button on rev A.

## Display, switches, sockets

| Qty | Ref | Part | Buy | $ | Ext |
|----:|-----|------|-----|--:|----:|
| 1 | DS1 | OLED 0.91" white SSD1306 4-pin | [AliExpress](https://www.aliexpress.com/w/wholesale-0.91-oled-ssd1306-128x32-white.html) (~$1.20) · [Amazon 4-pack](https://www.amazon.com/MakerFocus-Display-Module-SSD1306-3-3V-5V/dp/B08LQM9PQQ) (~$3.25 ea) | 1.20 | 1.20 |
| 8 | SW1–SW8 | LP hotswap socket (KS-27 / KS-33) | [Gateron](https://www.gateron.com/products/gateron-low-profile-switch-hot-swap-pcb-socket) · [beekeeb 5-pack](https://shop.beekeeb.com/products/gateron-low-profile-hotswap-socket-compatible-with-gateron-ks-27-switches-5pcs) | 0.16 | 1.25 |
| 8 | — | LP switch KS-33 (KS-27 OK) | [Gateron 35-pack](https://www.gateron.com/products/gateron-ks-33-low-profile-switch-set) ($7.84 / 35) | 0.22 | 1.79 |
| | | | | | **4.24** |

Sockets are on the schematic; switches plug in after fab. Confirm **white** OLED and pinout GND / VCC / SCL / SDA ([`lib/OLED.md`](lib/OLED.md)).

---

## Passives

Equivalents OK if value + package match. Crystal load caps stay **C0G**. 10 µF is **0805**; everything else **0603**.

| Qty | Value | Refs | Use | MPN | LCSC | Ext |
|----:|-------|------|-----|-----|------|----:|
| 2 | 22 Ω | R3, R4 | USB D+/D− series | 0603WAF220JT5E | [C23345](https://www.lcsc.com/product-detail/C23345.html) | 0.004 |
| 2 | 5.1 kΩ | R1, R2 | USB-C CC → GND | 0603WAF5101T5E | [C23186](https://www.lcsc.com/product-detail/C23186.html) | 0.017 |
| 16 | 1 kΩ | R5, R7, R16–R29 | BOOTSEL, XOUT, IQS Tx/Rx | 0603WAF1001T5E | [C21190](https://www.lcsc.com/product-detail/C21190.html) | 0.053 |
| 5 | 10 kΩ | R6, R13–R15, R30 | RUN PU, QT SNS, QSPI_SS PU | RC0603FR-0710KL | [C98220](https://www.lcsc.com/product-detail/C98220.html) | 0.025 |
| 2 | 4.7 kΩ | R8, R9 | I2C1 pull-ups | 0603WAF4701T5E | [C23162](https://www.lcsc.com/product-detail/C23162.html) | 0.006 |
| 1 | 220 Ω | R10 | MIDI IN LED | 0603WAF2200T5E | [C22962](https://www.lcsc.com/product-detail/C22962.html) | 0.005 |
| 1 | 10 Ω | R11 | MIDI OUT TX → Ring | 0603WAF100JT5E | [C22859](https://www.lcsc.com/product-detail/C22859.html) | 0.004 |
| 1 | 33 Ω | R12 | MIDI OUT 3V3 → Tip | 0603WAF330JT5E | [C23140](https://www.lcsc.com/product-detail/C23140.html) | 0.003 |
| 6 | 100 nF | C1, C3–C6, C14 | decoupling | CC0603KRX7R9BB104 | [C14663](https://www.lcsc.com/product-detail/C14663.html) | 0.013 |
| 4 | 1 µF | C2, C7–C9 | IQS VDDHI/VREG, RP2040 VREG | CC0603KRX5R8BB105 | [C14664](https://www.lcsc.com/product-detail/C14664.html) | 0.056 |
| 2 | 15 pF C0G | C12, C13 | crystal load | CL10C150JB8NNNC | [C1644](https://www.lcsc.com/product-detail/C1644.html) | 0.010 |
| 2 | 10 µF 0805 | C10, C11 | LDO in (VBUS) / out (3V3) | CL21A106KOQNNNE | [C1713](https://www.lcsc.com/product-detail/C1713.html) | 0.164 |
| | | | | | **Passives** | **0.36** |

Electronics (ICs + connectors + OLED + passives, no switches): **$11.09**. With switches/sockets: **$14.13**.

---

## PCB + cover

Board **118.95 × 63.15 mm**, **4-layer**. Quote at [JLCPCB](https://jlcpcb.com/) — qty-10 estimate **~$6.50** (fab + typical DHL split). ENIG is nicer for touch pads (~$5–8). Cover **~$2** filament (print, not a catalog part).

---

## Orders

1. **[LCSC](https://www.lcsc.com/)** — [RP2040](https://www.lcsc.com/product-detail/C2040.html), flash **[C179173](https://www.lcsc.com/product-detail/C179173.html)** (W25Q32JVSSIQ), [LDO](https://www.lcsc.com/product-detail/C51118.html), [ESD](https://www.lcsc.com/product-detail/C7519.html), [crystal](https://www.lcsc.com/product-detail/C20625731.html), [TLP2361](https://www.lcsc.com/product-detail/C107626.html), [diode](https://www.lcsc.com/product-detail/C2128.html), [EVQPUC02K](https://www.lcsc.com/product-detail/C79174.html), [BOOTSEL](https://www.lcsc.com/product-detail/C318884.html), all passives (C-codes above).
2. **[DigiKey](https://www.digikey.com/)** — [IQS572BLQNR](https://www.digikey.com/en/products/detail/azoteq-pty-ltd/IQS572BLQNR/7165004), [AT42QT2120-XUR](https://www.digikey.com/en/products/detail/microchip-technology/AT42QT2120-XUR/3678735), [USB4105-GF-A](https://www.digikey.com/en/products/detail/gct/USB4105-GF-A/11198441), [SJ1-3523N](https://www.digikey.com/en/products/detail/same-sky-formerly-cui-devices/SJ1-3523N/738689) ×2 per board.
3. [Gateron LP sockets](https://www.gateron.com/products/gateron-low-profile-switch-hot-swap-pcb-socket) + [KS-33 switches](https://www.gateron.com/products/gateron-ks-33-low-profile-switch-set); [Ali 0.91" OLED](https://www.aliexpress.com/w/wholesale-0.91-oled-ssd1306-128x32-white.html).
4. [JLCPCB](https://jlcpcb.com/) 4-layer, ~119 × 63 mm, qty 10.
