# M1 BOM (rev A)

Locks: [`../docs/goals.md`](../docs/goals.md). Switches: [`switches.md`](switches.md). OLED: [`lib/OLED.md`](lib/OLED.md).

**Hand-build only.** Bare PCB from fab. Hot air for RP2040 QFN; iron for the rest. One PCB — no harnesses, no 74HC14, no paid PCBA.

Packages: **0603** passives (0805 bulk OK), **SOIC-8** flash, **SOD-123** diodes, **H11L1M** (not 6N137).

---

## Parts (qty = 1 board)

| Qty | What | MPN | DigiKey | LCSC | Pkg | Status |
|-----|------|-----|---------|------|-----|--------|
| 3–5 | MCU | RP2040 | yes | C2040 | QFN-56 | locked |
| 2–3 | Flash 16 Mbit | W25Q16JVSSIQ / SSIM | yes | C131025 | SOIC-8 | candidate |
| 2 | 12 MHz xtal | ABM8-12.000MHZ-B2-T | yes | C596894 / C9002 | 3.2×2.5 | candidate |
| 1 | 3.3 V LDO | AP2112K-3.3TRG1 | yes | C51118 | SOT-23-5 | candidate |
| 2 | USB-C | USB4105-GF-A-120 | yes | C5184243 | SMT+TH | candidate |
| 2 | USB ESD | USBLC6-2SC6 | yes | C7519 | SOT-23-6 | candidate |
| 1 | Stick | RKJXV1220001 | rare | **C219778** | TH | locked |
| 8 | Diode | 1N4148W | yes | — | SOD-123 | locked |
| 3 | Tactile | B3F-1000 | yes | — | TH 6×6 | locked |
| 1 | MIDI IN opto | H11L1M | yes | C16587 | DIP-6 | candidate |
| 2 | TRS 3.5 mm | SJ1-3525N | yes | PJ-313 | TH R/A | candidate |
| 1 | OLED 0.91" white | Ali SSD1306 4-pin | — | — | TH | **have** (×5) |
| 8 | LP hotswap | Gateron LP socket | keeb shops | — | bottom SMD | ordered |
| 8 | LP switch | KS-27 / KS-33 | — | — | 2-pin | **have** |

Cut: 74HC14, Panic 4th button, PTC, FPC OLED, flying cables.

USB: use **C5184243** (GCT). Do not mix Hroparts **C165948** footprint.

### Passives (0603 unless noted)

| Qty | Value | Role |
|-----|-------|------|
| 2 | 27 Ω | USB D+/D− |
| 2 | 5.1 kΩ | CC1/CC2 → GND |
| 2–3 | 10 kΩ | BOOT / pulls |
| 2 | 4.7 kΩ | I2C (skip if OLED has them) |
| 1–2 | 220 Ω | MIDI IN |
| 1 | 1–10 kΩ | Opto pull-up |
| 1 | 10 Ω | MIDI OUT TX |
| 1 | 33 Ω | MIDI OUT tip |
| ~12 | 100 nF | Decoupling |
| 2 | 15–22 pF C0G | Crystal |
| 3–4 | 1–10 µF 0805 | LDO + bulk |
| 1 | 1 kΩ | QSPI_SS / BOOTSEL |

MIDI OUT = UART + resistors (Type A). No buffer IC.

---

## Buy / build

1. DigiKey: everything except stick, sockets, OLED.
2. LCSC: stick **C219778**.
3. 2-key LP socket coupon → then full bare PCB.
4. You assemble; case = PCB + plate + screws.
