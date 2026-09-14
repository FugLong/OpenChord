# M1 capacitive front panel (plan)

Locks / fallbacks: [`../docs/goals.md`](../docs/goals.md). Parts: [`bom.md`](bom.md).

**ICs:** **2× AT42QT2120-MMHR** (VQFN-20 3×3). DigiKey cut tape. Hot air with the RP2040.  
**Why two:** each chip’s slider/wheel mode uses only channels **0–2**, and the I2C address is fixed at **0x1C** — so one chip cannot do strip + wheel, and two chips cannot share one bus.

| Chip | Bus | Mode | Role |
|------|-----|------|------|
| A | **I2C0** (with OLED @ `0x3C`) | **Slider** (ch 0–2) | Touch **strip** — strum / chord-tone sparkle (Smart other hand; plugin can mirror CC/axis later) |
| B | **I2C1** | **Wheel** (ch 0–2) | Cap **stick** — absolute angle → same 8 color seats + HOME on lift |
| Either | spare SNS 3–11 | Discrete keys | **System pads** — Key / Shift / mode (and Panic if wanted) |

Fall back during design: Alps stick and/or TH tactiles if the flat PCB feel fails. Chord pads stay **Gateron LP**.

## How a chip works

1. Top copper = electrodes (strip triangles, wheel ring, or button pads).
2. Each net → one **SNS** pin (SNS0…SNS11). Short traces; series Rs only if the datasheet asks.
3. Chip measures capacitance; RP2040 reads over I2C. Firmware does seats / strum mapping.
4. RP2040 does **not** sense touch itself.

### Slider / wheel (channels 0–2 only)

Three **interleaved** electrodes (triangles / chevrons), not three coarse boxes. Relative strengths → continuous **0–255**. Slider = linear strip; wheel = same idea bent into a ring (absolute angle, not just scroll direction). Empty center of the wheel ring = deadzone.

Remaining channels (up to 9) are normal keys — or unused / guard / proximity.

```
Finger
  ↓  (cover opening + soldermask, or ENIG frets)
[ interleaved electrodes ]  → SNS0–2  → QT2120  → I2C → RP2040
[ discrete key pads      ]  → SNS3+   → same chip
```

## Soldermask vs gold

| Finish | Look | Fab |
|--------|------|-----|
| **Mask over pads** (default) | Pad-shaped bumps in board color | Standard — works fine |
| **ENIG + mask open** | Gold rectangles / frets | Open mask only on electrodes |

One soldermask color for the whole PCB either way. No SoftPot film.

## Cover

3D-printed cover with **openings** over strip, wheel rim, and system pads — clearer targets, fewer palm hits. Electrode size under plastic: plan ~**8–12 mm** pads, **≥2–3 mm** gaps for discrete keys; follow Microchip QTouch layout notes for slider/wheel tooth spacing.

## Layout notes

- Ground pour around electrodes (keepout under sense copper). QT2120 close to its electrodes.
- Optional **CHANGE** pin per chip → GPIO so firmware can skip polling when idle.
- Passives / decoupling: Microchip datasheet at schematic time.

## Coupon before full board

Bring up strip-only and/or wheel-only coupons before freezing the product PCB. Keep C219778 footprint available until the wheel is proven.
