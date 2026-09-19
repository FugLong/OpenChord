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

**3× system EVQPUC02K** → GPIO (internal pull-up). Roles **TBD** — not capacitive, not Gaterons. **BOOTSEL** is a separate strap on QSPI_SS (see [`support.md`](support.md)).

### Pad map (firmware)

| Pad | Pro | Smart |
|-----|-----|-------|
| 0–3 | Dim Min Maj Sus | I ii iii IV |
| 4–7 | 6 m7 M7 9 | V vi vii I↑ |

Physical layout is still a **2×4** cluster; numbering is firmware-defined.

### GPIO map (rev A schematic)

See [`support.md`](support.md) §8 for the full table. Firmware must use **I2C1** (GPIO10/11), not I2C0.

| Function | GPIO |
|----------|------|
| KEY (SW1–8 Gateron) | 24, 18, 25, 17, 2, 13, 6, 12 |
| SYS (SW9, SW10, SW11 EVQ) | 4, 5, 3 |
| I2C1 SDA/SCL | 10 / 11 — OLED `0x3C` + QT2120 strip @ `0x1C` + IQS572 @ `0x74` |
| IQS572_RDY / NRST | 8 / 9 |
| UART0 TX/RX (MIDI) | 0 / 1 |
| BOOTSEL | **Not a GPIO** — TS-1187A + **R5 1 kΩ** on **QSPI_SS**, **R30 10 kΩ** SS→3V3 |
| STICK_X/Y | **Only if Alps fallback** — ADC on GPIO26–29 |

QT2120 **CHANGE** is unconnected. Spare: GPIO7, 14–16, 19–23.

## Layout

2×4 left-hand cluster; thumb **trackpad** (or Alps stick fallback); touch **strip** on a free edge; **3× system EVQs** (roles TBD); USB-C on edge; TRS rear/side; bottom keepout for case.
