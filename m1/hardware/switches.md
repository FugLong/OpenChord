# M1 switches — Gateron LP, direct GPIO

Parts: [`bom.md`](bom.md). Feel: [`../docs/goals.md`](../docs/goals.md). Cap front: [`touch.md`](touch.md).

## Socket family

**Gateron LP KS-27 / KS-33** only (2 electrical pins). **Not** MX (CPG1511). **Not** Choc. Sockets from Gateron / keeb shops — not DigiKey.

KS-27 and KS-33 share a compatible footprint. Wrong family = scrap board.

## Stack-up

```
[keycap] [LP switch]  ← press from top
[plate]
── PCB top ──  NPTH for legs
── PCB bottom ──  LP hotswap socket (SMD)
```

Solder sockets on the **bottom**. Press switches in after plate. Coupon 1–2 keys before full fab.

Footprints to try (verify on coupon): [siderakb/key-switches.pretty](https://github.com/siderakb/key-switches.pretty) `SW_Gateron_LowProfile_HotSwap_*`.

## Wiring — direct GPIO (no matrix)

Eight pads → **eight GPIOs**. One switch pin → GPIO, other → GND.

- Enable **internal pull-up** on each key GPIO (~50–80 kΩ on RP2040). No external pull-up resistors.
- Pressed = low. Firmware debounce.
- **No diodes** — ghosting is a matrix problem; each key has its own line.
- Multi-hold works.

Key / Shift / mode stay capacitive (spare QT2120 SNS) or TH tactiles on dedicated GPIOs if we fall back — not part of the eight Gaterons.

### Pad map (firmware)

| Pad | Pro | Smart |
|-----|-----|-------|
| 0–3 | Dim Min Maj Sus | I ii iii IV |
| 4–7 | 6 m7 M7 9 | V vi vii I↑ |

Physical layout is still a **2×4** cluster; numbering is firmware-defined.

### GPIO sketch (not final pins)

Avoid USB, QSPI, crystal pins.

| Function | Notes |
|----------|--------|
| KEY0–KEY7 | Direct to Gaterons; internal pull-up |
| I2C0 SDA/SCL | OLED (`0x3C`) + QT2120 A (strip / slider) @ `0x1C` |
| I2C1 SDA/SCL | QT2120 B (wheel + system pads) @ `0x1C` |
| UART TX/RX | MIDI out / in |
| TOUCH_CHANGE_A/B | Optional, per-chip CHANGE |
| STICK_X/Y | **Only if Alps fallback** — ADC on GPIO26–29 |

~16 GPIOs used with optional CHANGE pins — fine on RP2040 (~30 available).

## Layout

2×4 left-hand cluster; thumb **wheel** (or Alps stick fallback); touch **strip** on a free edge; system pads near thumb/edge under cover openings; USB-C on edge; TRS rear/side; bottom keepout for case.
