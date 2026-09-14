# M1 switches — Gateron LP + matrix

Parts: [`bom.md`](bom.md). Feel: [`../docs/goals.md`](../docs/goals.md).

## Socket family

**Gateron LP KS-27 / KS-33** only (2 electrical pins). **Not** MX (CPG1511). **Not** Choc. Sockets from Gateron / keeb shops — not DigiKey.

KS-27 and KS-33 share a compatible footprint. Wrong family = scrap board.

## Stack-up

```
[keycap] [LP switch]  ← press from top
[plate]
── PCB top ──  NPTH for legs
── PCB bottom ──  LP hotswap socket (SMD) + diode → matrix
```

Solder sockets on the **bottom**. Press switches in after plate. Coupon 1–2 keys before full fab.

Footprints to try (verify on coupon): [siderakb/key-switches.pretty](https://github.com/siderakb/key-switches.pretty) `SW_Gateron_LowProfile_HotSwap_*`.

## Matrix 2×4

8 pads → 4 cols × 2 rows = 6 GPIOs + 8 diodes.

- Scan: **rows out**, **columns in** (pull-ups).
- Diode: **anode → row**, **cathode → column** (match firmware).
- Multi-hold needs diodes — mandatory.

Tactiles (Key / Shift / mode) = dedicated GPIOs to GND, **not** in the matrix.

### Pad map (firmware)

| Cell | Pro | Smart |
|------|-----|-------|
| R0C0–R0C3 | Dim Min Maj Sus | I ii iii IV |
| R1C0–R1C3 | 6 m7 M7 9 | V vi vii I↑ |

### GPIO sketch (not final pins)

Avoid USB, QSPI, crystal pins.

| Function | Notes |
|----------|--------|
| ROW0–1, COL0–3 | Matrix |
| STICK_X/Y | ADC, C219778 |
| I2C SDA/SCL | OLED **+ AT42QT2120** (same bus) |
| UART TX/RX | MIDI out / in |
| KEY, SHIFT, MODE | Tactiles |
| TOUCH_CHANGE | Optional, QT2120 change/IRQ-style pin if used |

## Layout

2×4 left-hand cluster; stick at thumb; **tentative touch strip** along a free edge (I2C electrodes); USB-C on edge; TRS rear/side; diode next to each socket; bottom keepout for case.
