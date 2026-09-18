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

**3× system EVQ-PUA02K** → GPIO (internal pull-up). Roles **TBD** — not capacitive, not Gaterons. **BOOTSEL** is a separate strap on QSPI_SS (see [`support.md`](support.md)).

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
| SYS0–SYS2 | 3× EVQ → GPIO; roles TBD |
| I2C0 SDA/SCL | OLED (`0x3C`) + QT2120 (**strip only**) @ `0x1C` + IQS572 (trackpad) @ `0x74` |
| IQS572_RDY | Required — Azoteq ready line |
| TOUCH_CHANGE | Optional QT2120 CHANGE |
| UART TX/RX | MIDI out / in |
| BOOTSEL | **Not a GPIO** — EVQ + 1 kΩ on **QSPI_SS** (UF2 at reset) |
| STICK_X/Y | **Only if Alps fallback** — ADC on GPIO26–29 |

~14–16 GPIOs with RDY / CHANGE — fine on RP2040 (~30 available).

## Layout

2×4 left-hand cluster; thumb **trackpad** (or Alps stick fallback); touch **strip** on a free edge; **3× system EVQs** (roles TBD); USB-C on edge; TRS rear/side; bottom keepout for case.
