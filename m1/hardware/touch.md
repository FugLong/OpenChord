# M1 capacitive touch strip (tentative)

**IC:** AT42QT2120-MMHR (VQFN-20 3×3). DigiKey cut tape. Hot air with the RP2040.  
**Role:** Smart-mode other hand — strum + chord-tone / sparkle. Plugin can mirror via CC/axis later.  
Parts list: [`bom.md`](bom.md).

## How it works

1. Top copper = a **row of pads** (electrodes), e.g. 3–12 along a strip.
2. Each pad is a normal PCB trace to one **SNS** pin on the QT2120 (SNS0…SNS11). Short, direct, no series resistors unless the datasheet asks.
3. QT2120 measures capacitance on each pin over I2C (same bus as OLED). Firmware reads slider position / which segment is touched.
4. RP2040 does not sense touch itself — only talks to the QT2120.

```
Finger
  ↓  (through soldermask)
[ pad0 ] [ pad1 ] [ pad2 ] …   ← top copper electrodes
   |        |        |
   +--------+--------+----→ SNS0, SNS1, SNS2… on QT2120
                              I2C → RP2040
```

## Soldermask vs gold

| Finish | Look | Fab |
|--------|------|-----|
| **Mask over pads** (default) | Pad-shaped bumps in board color | Standard — works fine |
| **ENIG + mask open** | Gold rectangles | Same board color elsewhere; open mask only on electrodes |

One soldermask color for the whole PCB either way. No clear mask, no SoftPot.

## Layout notes

- Ground pour around the strip (keepout under pads). Follow Microchip QTouch electrode guidelines when drawing.
- QT2120 close to the strip; I2C shared with OLED (`0x3C`); QT2120 addr per datasheet wiring.
- Optional CHANGE pin → GPIO for “something touched” without polling.
