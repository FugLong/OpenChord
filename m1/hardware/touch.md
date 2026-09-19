# M1 capacitive front panel (**locked**)

Locks / fallbacks: [`../docs/goals.md`](../docs/goals.md). Parts: [`bom.md`](bom.md).

## Locked ICs

| Role | MPN | DigiKey | Pkg | Bus |
|------|-----|---------|-----|-----|
| **Trackpad** (color stick) | **IQS572BLQNR** | [7165004](https://www.digikey.com/en/products/detail/azoteq-pty-ltd/IQS572BLQNR/7165004) | QFN-28 **4×4** (**0.5 mm** pitch) | I²C **`0x74`**, needs **RDY** GPIO |
| **Strip** (strum / sparkle) | **AT42QT2120-XUR** | [XUR](https://www.digikey.com/en/products/detail/microchip-technology/AT42QT2120-XUR/3678735) (TSSOP-20, tape/reel — prefer) · [MMHR](https://www.digikey.com/en/products/detail/microchip-technology/AT42QT2120-MMHR/3678733) (VQFN-20 alt) | same die | I²C **`0x1C`** |

**One of each.** Not 2× QT2120. IQS572 = XY trackpad. QT2120 = **strip only** (SNS0–2). System buttons = mechanical EVQ → GPIO (**roles TBD**) — not capacitive.

Addresses differ → **OLED (`0x3C`) + QT2120 + IQS572 share I2C1** (GPIO10 SDA / GPIO11 SCL). Pull-ups once (**R8/R9 4.7 kΩ**). IQS572 **RDY → GPIO8** (required). **NRST → GPIO9**. QT2120 **CHANGE** is open (poll I²C).

Fall back: Alps stick (C219778) if trackpad feel fails. Chord pads stay **Gateron LP**. System buttons stay EVQ.

---

## What each chip does

### IQS572 — trackpad (stick)

Mutual-cap **Tx columns + Rx rows**. Copper = diamond matrix from the captouch plugin (or hand layout per Azoteq AZD068). Chip outputs **XY** (+ gestures) over I²C. Firmware maps position → **8 color seats + HOME** (center / no-touch deadzone). Up to **9×8** channels; use a small stick-sized grid (e.g. 5×4 / 6×5), leave unused Tx/Rx unconnected per datasheet rules.

### AT42QT2120 — strip only

Self-cap. **Slider** on SNS0–2 = three interleaved electrodes → continuous **0–255** (strum / sparkle). Unused SNS left open. System buttons are **not** on this chip.

```
Finger → cover opening + soldermask (rev A HASL; not ENIG)
  trackpad diamonds → Tx/Rx (+ 1 kΩ R16–R29) → IQS572 → I2C1 → RP2040
  strip triangles    → SNS0–2 (+ 10 kΩ R13–R15) → QT2120 → I2C1 → RP2040
  3× system EVQ      → GPIO (roles TBD)
```

## Soldermask vs gold

| Finish | Look | Fab |
|--------|------|-----|
| **HASL + mask over pads** (rev A proto) | Pad-shaped bumps in board color | What we ordered |
| **ENIG + mask open** | Gold frets / pad outlines | Later SKU only if feel needs gold |

## Cover

3D-printed cover with openings over **trackpad** and **strip**. System EVQs get their own openings (not copper pads).

## Layout notes

- Keepout under **sense** copper on internal GND/PWR planes; ground pour around (not under) electrodes. Slider **GND end chevrons** (U2 pads 4–5) may sit outside keepout so parts can share that area — that is intentional.
- IQS572 and QT2120 **close** to their electrodes; short sense traces; **series R on every sense line**:
  - Strip: **R13–R15 10 kΩ** (QT2120 datasheet Rs 4.7–20 kΩ)
  - Trackpad 7×7: **R16–R29 1 kΩ** — electrode silk Rx1–7/Tx1–7 → chip **Rx0–6 / Tx0–6** (Rx7, Tx7, Tx8 NC)
- Decoupling: IQS572 **C2 1 µF on VDDHI**, **C7 1 µF on VREG** (VREG is the internal regulator output — cap to GND only). Optional 100 pF across C7 skipped on rev A.
- IQS572 is **NRFND** at Azoteq but DigiKey-stocked — acceptable for M1; do not redesign around vaporware 7211E.

---

## Eval / breakouts (buy before fab)

### IQS572 (preferred → fallback)

1. **Official:** [IQS572EV02](https://www.azoteq.com/product/iqs572-b000/) Arduino-shield trackpad (8×8) — order from Azoteq / Seltech if DigiKey empty. Pair with **CT210A** or **DS200** USB streamer + [IQS5xx-B000 GUI](https://www.azoteq.com/).
2. **Ready module (best buy):** [TPS43-201A-S](https://www.digikey.com/en/products/detail/azoteq-pty-ltd/TPS43-201A-S/7164940) — 43×40 mm pad with **IQS572 on board** (FPC). Wire to Zero; prove XY + seat mapping before any coupon.
3. **DIY chip on adapter:** only if you cannot get TPS43/EV02 — need a **QFN-28 4×4 / 0.5 mm → DIP** adapter (not 0.4 mm; Chip Quik **IPC0042 is 0.4 mm and wrong**). Prefer coupon with the IC soldered direct.

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
2. **Strip coupon:** QT2120 + interleaved strip + cover opening (no Key/Shift copper).
3. Optional: one **combo coupon** (both ICs) once each alone passes.

Sweep **one variable per coupon** if possible: pitch / overlay / mask-vs-ENIG. Do **not** change five things between fabs.

### Phase 2 — checklist before product Gerbers

- [ ] Trackpad: all seats reachable; HOME center reliable; no palm on rim while resting hand
- [ ] Strip: end-to-end travel; no dead third; works with intended cover
- [ ] System EVQs: clean GPIO presses; no ghost with trackpad palm
- [ ] EMI: USB plugged, MIDI TRS connected, OLED on — touch still stable
- [ ] I2C: OLED + both touch ICs on one bus at once (addr `0x3C` / `0x1C` / `0x74`)
- [ ] Power: brown-out / plug cycle; IQS572 RDY still sane after reset
- [ ] Mechanical: cover openings align; finger never on bare copper; edge buttons reachable
- [ ] Alps stick footprint DNP fallback still OK if trackpad fails

### Phase 3 — product PCB

Only after Phase 1–2 pass. First product fab is **assembly + layout** risk (USB, TRS, Gateron, RP2040), not “will capacitive touch exist.”

**Truth:** you will not get “zero respins” by staring at KiCad. You get close by **paying for coupons** and freezing electrode geometry before the big board.
