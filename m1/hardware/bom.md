# M1 BOM (rev A)

Locks: [`../docs/goals.md`](../docs/goals.md). Switches: [`switches.md`](switches.md). OLED: [`lib/OLED.md`](lib/OLED.md). Touch: [`touch.md`](touch.md).

**Hand-build only.** Bare PCB. Hot air: RP2040 + **IQS572** + **QT2120**. Iron: the rest. One board, no harnesses, no SoftPot, no paid PCBA.

Packages: **0603** (0805 bulk OK), **SOIC-8** flash, **H11L1M**, **IQS572BLQNR** QFN-28 4×4, **AT42QT2120** VQFN-20 or TSSOP-20.

**Cost (small batch ~10–25):** parts + PCB + printed cover ≈ **$25–35**/unit (switches included); ~$22–30 if switches already paid for. Street target under **$100** with margin.

---

## Parts (qty = 1 board)

| Qty | What | MPN | DigiKey | LCSC | Pkg | Status |
|-----|------|-----|---------|------|-----|--------|
| 3–5 | MCU | RP2040 | yes | C2040 | QFN-56 | locked |
| 2–3 | Flash 16 Mbit | W25Q16JVSSIQ / SSIM | yes | C131025 | SOIC-8 | candidate |
| 2 | 12 MHz xtal | ABM8-12.000MHZ-B2-T | yes | C596894 / C9002 | 3.2×2.5 | candidate |
| 1 | 3.3 V LDO | AP2112K-3.3TRG1 | yes | C51118 | SOT-23-5 | candidate |
| 2 | USB-C | **USB4105-GF-A** | [11198441](https://www.digikey.com/en/products/detail/gct/USB4105-GF-A/11198441) | **C3020560** | SMT+TH | **locked** — KiCad `oc-usb:USB4105-GF-A`; 20k cycles |
| 2 | USB ESD | USBLC6-2SC6 | yes | C7519 | SOT-23-6 | candidate |
| 1 | Cap trackpad | **IQS572BLQNR** | [7165004](https://www.digikey.com/en/products/detail/azoteq-pty-ltd/IQS572BLQNR/7165004) | **C3827635** | QFN-28 4×4 | **locked** — KiCad `oc-touch:IQS572-BL-QNR` |
| 1 | Cap strip / Key / Shift | **AT42QT2120-XUR** | [XUR](https://www.digikey.com/en/products/detail/microchip-technology/AT42QT2120-XUR/3678735) | **C1522278** (same die/pkg) | TSSOP-20 | **locked** — reel pack; KiCad `oc-touch:AT42QT2120-XUR`; MMHR DigiKey-only alt |
| 1 | MIDI IN opto | H11L1M | yes | C16587 | DIP-6 | candidate |
| 2 | TRS 3.5 mm | **SJ1-3523N** | [738689](https://www.digikey.com/en/products/detail/same-sky-formerly-cui-devices/SJ1-3523N/738689) | **C20182914** | TH R/A, **0 switches** | **locked** — KiCad `oc-trs:SJ1-3523N`; Type A; tip/ring always live |
| 1 | Mode btn | **EVQ-PUA02K** | [286334](https://www.digikey.com/en/products/detail/panasonic-industry/EVQ-PUA02K/286334) | **C128539** | SMD side ~4.7×3.5×1.65 | **locked** — KiCad `oc-btn:EVQ-PUA02K`; edge Pro/Smart |
| 1 | OLED 0.91" white | Ali SSD1306 4-pin | — | — | TH | **have** (×5) |
| 8 | LP hotswap | Gateron LP socket | keeb shops | — | bottom SMD | ordered |
| 8 | LP switch | KS-27 / KS-33 | — | — | 2-pin | **have** |

### Fallback (keep available; not default)

| Qty | What | MPN | DigiKey | LCSC | Pkg | When |
|-----|------|-----|---------|------|-----|------|
| 1 | Stick | RKJXV1220001 | rare | **C219778** | TH | If trackpad feel fails (KiCad lib already here) |
| 3 | Tactile | B3F-1000 | yes | — | TH 6×6 | If Key/Shift cap pads fail |

Cut: SoftPot, 74HC14, matrix diodes (keys are direct GPIO), Panic-as-extra-tactile (use Shift or spare SNS), PTC, FPC OLED, flying cables, paid PCBA. Second QT2120 (old wheel plan).

### Passives (0603 unless noted)

| Qty | Value | Role |
|-----|-------|------|
| 2 | 27 Ω | USB D+/D− |
| 2 | 5.1 kΩ | CC1/CC2 → GND |
| 2–3 | 10 kΩ | BOOT / pulls |
| 2 | 4.7 kΩ | I2C0 (OLED + QT2120 + IQS572) |
| 1–2 | 220 Ω | MIDI IN |
| 1 | 1–10 kΩ | Opto pull-up |
| 1 | 10 Ω | MIDI OUT TX |
| 1 | 33 Ω | MIDI OUT tip |
| ~14 | 100 nF | Decoupling (incl. IQS572 + QT2120) |
| 2 | 15–22 pF C0G | Crystal |
| 3–4 | 1–10 µF 0805 | LDO + bulk |
| 1 | 1 kΩ | QSPI_SS / BOOTSEL |

Touch extras (series R, VREG caps): Azoteq IQS5xx + Microchip QT2120 datasheets at schematic time. MIDI OUT = UART + resistors (Type A).

### Eval / bring-up (not in product BOM)

| What | Why | Link |
|------|-----|------|
| **TPS43-201A-S** | Ready IQS572 trackpad module → RP2040-Zero | [DigiKey](https://www.digikey.com/en/products/detail/azoteq-pty-ltd/TPS43-201A-S/7164940) |
| IQS572EV02 (+ CT210A/DS200) | Official Azoteq shield + GUI | [Azoteq IQS572](https://www.azoteq.com/product/iqs572-b000/) |
| QFN-28 4×4 **0.5 mm** → DIP | Only if hand-soldering bare IQS572; **not** Chip Quik IPC0042 (that is 0.4 mm) | — |
| Touchy Subject / QT2120 | Strip/key bring-up | [Lectronz](https://www.lectronz.com/products/touchy-subject) / DigiKey **XUR** |

Details: [`touch.md`](touch.md).

---

## Buy / build

1. DigiKey: core + **IQS572BLQNR** + **AT42QT2120-XUR** (or MMHR).
2. Eval: **TPS43** and/or QT2120 breakout → Zero.
3. Cap coupons → bare product PCB → hand assemble → plate + case.
