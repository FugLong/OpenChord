# M1 capacitive front panel (**locked**)

Locks / fallbacks: [`../docs/goals.md`](../docs/goals.md). Parts: [`bom.md`](bom.md).

## Locked ICs

| Role | MPN | DigiKey | Pkg | Bus |
|------|-----|---------|-----|-----|
| **Trackpad** (color stick) | **IQS572BLQNR** | [7165004](https://www.digikey.com/en/products/detail/azoteq-pty-ltd/IQS572BLQNR/7165004) | QFN-28 **4×4** (0.4 mm) | I²C **`0x74`**, needs **RDY** GPIO |
| **Strip + Key / Shift** | **AT42QT2120-XUR** | [XUR](https://www.digikey.com/en/products/detail/microchip-technology/AT42QT2120-XUR/3678735) (TSSOP-20, tape/reel — prefer) · [MMHR](https://www.digikey.com/en/products/detail/microchip-technology/AT42QT2120-MMHR/3678733) (VQFN-20 alt) | same die | I²C **`0x1C`** |

**One of each.** Not 2× QT2120. IQS572 does real XY (mutual diamond). QT2120 does strip (slider ch 0–2) + **Key / Shift** on spare SNS. Mode = edge **EVQ-PUA02K** (not capacitive).

Addresses differ → **OLED (`0x3C`) + QT2120 + IQS572 can share I2C0**. Pull-ups once. Wire IQS572 **RDY** to a GPIO (required for clean comms). Optional QT2120 **CHANGE** → GPIO.

Fall back: Alps stick (C219778) and/or TH tactiles if feel fails. Chord pads stay **Gateron LP**.

---

## What each chip does

### IQS572 — trackpad (stick)

Mutual-cap **Tx columns + Rx rows**. Copper = diamond matrix from the captouch plugin (or hand layout per Azoteq AZD068). Chip outputs **XY** (+ gestures) over I²C. Firmware maps position → **8 color seats + HOME** (center / no-touch deadzone). Up to **9×8** channels; use a small stick-sized grid (e.g. 5×4 / 6×5), leave unused Tx/Rx unconnected per datasheet rules.

### AT42QT2120 — strip + buttons

Self-cap. **Slider** on SNS0–2 = three interleaved electrodes → continuous **0–255** (strum / sparkle). Remaining SNS = discrete **Key / Shift** pads (and optional extras). Mode is mechanical — see [`lib/BTN.md`](lib/BTN.md).

```
Finger → cover opening + mask/ENIG
  trackpad diamonds → Tx/Rx → IQS572 → I2C0 → RP2040
  strip triangles    → SNS0–2 → QT2120 → I2C0 → RP2040
  Key / Shift pads     → SNS3+  → QT2120 → same
```

## Soldermask vs gold

| Finish | Look | Fab |
|--------|------|-----|
| **Mask over pads** (default) | Pad-shaped bumps in board color | Standard |
| **ENIG + mask open** | Gold frets / pad outlines | Open mask only on electrodes |

## Cover

3D-printed cover with openings over **trackpad**, **strip**, and **Key / Shift**. Discrete pads ~**8–12 mm**, gaps **≥2–3 mm**. Overlay over electrodes **≥0.5 mm** (datasheets — bare copper is not a valid test). Mode button is a separate edge opening for the PUA plate.

## Layout notes

- Keepout under sense copper; ground pour around (not under) electrodes.
- IQS572 and QT2120 **close** to their electrodes; short sense traces; series R per datasheet (~2 kΩ mutual / ~560 Ω self ballpark).
- Decoupling / VREG caps: Azoteq + Microchip datasheets at schematic time.
- IQS572 is **NRFND** at Azoteq but DigiKey-stocked — acceptable for M1; do not redesign around vaporware 7211E.

---

## Eval / breakouts (buy before fab)

### IQS572 (preferred → fallback)

1. **Official:** [IQS572EV02](https://www.azoteq.com/product/iqs572-b000/) Arduino-shield trackpad (8×8) — order from Azoteq / Seltech if DigiKey empty. Pair with **CT210A** or **DS200** USB streamer + [IQS5xx-B000 GUI](https://www.azoteq.com/).
2. **Ready module (best buy):** [TPS43-201A-S](https://www.digikey.com/en/products/detail/azoteq-pty-ltd/TPS43-201A-S/7164940) — 43×40 mm pad with **IQS572 on board** (FPC). Wire to Zero; prove XY + seat mapping before any coupon.
3. **DIY chip on adapter:** solder IQS572BLQNR onto a **QFN-28 4×4 / 0.4 mm → DIP** adapter, then fly-wire to a hand-etched or JLCPCB **coupon** with only the diamond matrix + 0603s:
   - [Chip Quik IPC0042](http://www.chipquik.com/store/product_info.php?products_id=3100042) (~$9)
   - [Artekit QFN-28 4×4 P0.40](https://www.artekit.eu/products/breakout-boards/bbadapters/qfn-to-dip/qfn-28-4x4-p040-to-dip-adapter/)

### AT42QT2120

1. **[Touchy Subject](https://www.lectronz.com/products/touchy-subject)** — Qwiic/STEMMA QT2120 breakout (hand-solderable TSSOP design; open source).
2. **Zio Qwiic AT42QT2120** (Smart Prototyping) — module + optional wheel/key panels.
3. **DIY:** buy **AT42QT2120-XUR** (TSSOP-20), dead-bug or TSSOP→DIP adapter onto a strip coupon. Product PCB can still use MMHR if preferred.

### Host for bring-up

RP2040-Zero (`m1/proto-rp2040`) or Arduino Uno (EV02 shield). Log XY / slider / keys over USB serial before trusting the product board.

---

## Pre-fab testing (expensive PCBs — earn one fab)

Cap copper is easy to draw and easy to **feel** wrong (dead zones, palm hits, jitter, cover air gap). Treat feel as a **coupon problem**, not a full-board problem.

### Phase 0 — silicon + software (no custom PCB)

| Test | How | Pass |
|------|-----|------|
| IQS572 talks | EV02 / TPS43 / adapter → I2C `0x74`, RDY polled, XY streams | Stable XY at 3.3 V |
| QT2120 talks | Breakout → I2C `0x1C`, slider 0–255, keys debounce | Clean lift/touch |
| Stick mapping | Firmware: XY → polar or 8 seats + HOME deadzone | Matches gesture lock in goals |
| Strip mapping | Slider → strum / sparkle | Useful in Smart; no stuck values |

### Phase 1 — **cheap coupons** (JLCPCB ~$5–15, 2–5 pcs)

Order **tiny boards**, not the product:

1. **Trackpad coupon:** IQS572 footprint + diamond matrix sized like the final stick + cover stack (same overlay thickness you will ship) + I2C/RDY header to Zero.
2. **Strip + pads coupon:** QT2120 + interleaved strip + Key/Shift pads + cover openings.
3. Optional: one **combo coupon** (both ICs) once each alone passes.

Sweep **one variable per coupon** if possible: pitch / overlay / mask-vs-ENIG. Do **not** change five things between fabs.

### Phase 2 — checklist before product Gerbers

- [ ] Trackpad: all seats reachable; HOME center reliable; no palm on rim while resting hand
- [ ] Strip: end-to-end travel; no dead third; works with intended cover
- [ ] Key / Shift pads: no accidental hits from trackpad / strip use
- [ ] EMI: USB plugged, MIDI TRS connected, OLED on — touch still stable
- [ ] I2C: OLED + both touch ICs on one bus at once (addr `0x3C` / `0x1C` / `0x74`)
- [ ] Power: brown-out / plug cycle; IQS572 RDY still sane after reset
- [ ] Mechanical: cover openings align; finger never on bare copper; mode PUA reachable on edge
- [ ] Fallback footprints still on schematic (Alps / Key-Shift tactiles) but DNP

### Phase 3 — product PCB

Only after Phase 1–2 pass. First product fab is **assembly + layout** risk (USB, TRS, Gateron, RP2040), not “will capacitive touch exist.”

**Truth:** you will not get “zero respins” by staring at KiCad. You get close by **paying for coupons** and freezing electrode geometry before the big board.
