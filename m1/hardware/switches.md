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

Footprints **in-repo** (ai03 MX_V2, MIT): see [`lib/GATERON.md`](lib/GATERON.md).

- Schematic: **`oc-gateron:SW_Gateron_LP`**
- PCB: **`oc-gateron:Gateron-KS33-Hotswap-1U`** — then **flip** so socket is on **B.Cu** (bottom)
- Backup solder-only: `Gateron-KS33-Solderable-1U`
- 3D: socket + switch (`Gateron-KS33-Socket.step`, `Gateron-KS33-Switch.step`) — see [`lib/GATERON.md`](lib/GATERON.md)

## Wiring — direct GPIO (no matrix)

Eight pads → **eight GPIOs**. One switch pin → GPIO, other → GND.

- Enable **internal pull-up** on each key GPIO (~50–80 kΩ on RP2040). No external pull-up resistors.
- Pressed = low. Firmware debounce.
- **No diodes** — ghosting is a matrix problem; each key has its own line.
- Multi-hold works.

**Key / Shift** = capacitive (spare QT2120 SNS; TH tactiles if we fall back). **Mode** = edge **EVQ-PUA02K** (GPIO). None of these are Gaterons.

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
| MODE | EVQ-PUA02K → GPIO; internal pull-up |
| I2C0 SDA/SCL | OLED (`0x3C`) + QT2120 (strip / Key / Shift) @ `0x1C` + IQS572 (trackpad) @ `0x74` |
| IQS572_RDY | Required — Azoteq ready line |
| TOUCH_CHANGE | Optional QT2120 CHANGE |
| UART TX/RX | MIDI out / in |
| STICK_X/Y | **Only if Alps fallback** — ADC on GPIO26–29 |

~14–16 GPIOs with RDY / CHANGE — fine on RP2040 (~30 available).

## Layout

2×4 left-hand cluster; thumb **trackpad** (or Alps stick fallback); touch **strip** on a free edge; Key/Shift pads near thumb under cover openings; **mode** on board edge (PUA); USB-C on edge; TRS rear/side; bottom keepout for case.
