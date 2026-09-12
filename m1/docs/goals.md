# OpenChord M1 — goals and plan

Written September 2026 so we do not lose the decisions that led here. This is the product spec for v1. Firmware lives under `m1/`. Do not edit `archive/s1-daisy/`.

**Working name:** OpenChord M1 (`OPENCHORD / m1`)

**Family:** OpenChord. **M** = MIDI tools. **S** = studio. Numbers are generations. The jambox, when it exists, is **S1**. Later we can do M2 / S2 without renaming anything.

---

## Why this exists

The original OpenChord was a portable Daisy Seed jambox: synth, tracks, looper, sampler, OLED, battery, 11 keys, joystick. A prototype was built. It taught us a lot. It is not the first thing to ship.

Reasons we paused the full dream:

- Daisy Seed audio on the proto never got clean (wiring, PowerBoost, grounds — the module fight was not worth another year).
- The kitchen-sink spec (four live engines, USB host + device, granular + wavetable + analog, audio looper) does not match a first sellable board.
- The author is a **DAW person**. MIDI into a computer is how the music actually gets made. A DAWless studio-in-a-brick was a fantasy of a device that would finally make DAWless feel good. At the end of the day the DAW won.
- The interesting idea was never the synth. It was **smart, dynamic chord mapping** that stays musical.

So: a smaller, MIDI-only product that can live on a custom PCB and actually be sold. S1 stays in the family for later.

The Daisy tree is archived at [`../../archive/s1-daisy/`](../../archive/s1-daisy/). Do not treat it as S1 source.

---

## What M1 is

A **man-in-the-middle MIDI enhancer**. Feed it MIDI from a real keyboard or the DAW. It sends MIDI back transformed: chords, voicings, extensions, voice leading.

- No audio.
- No USB host. The PC is the USB host. M1 is a USB MIDI **device**.
- TRS MIDI in and TRS MIDI out (plus USB MIDI in/out).
- Bus-powered from USB-C. **No battery** on v1. If TRS-only gear needs it, plug USB-C into power with no data.
- Small enough to sit on a desk next to a keyboard and disappear into a session.

One-line pitch: *sits between your keys and Ableton and makes you sound like you know theory.*

Ambition: the most helpful chord/MIDI device we have used. Not a quality roulette. It should voice well, stay in key, and later help with **where to go next** (progressions, ideas). Voicing that never sounds stupid is v1. Progression-smart is the north star; do not fake it with random borrowed chords. Write the model in [chord-engine.md](chord-engine.md).

---

## What M1 is not

- Not a synth, sampler, looper, or drum machine.
- Not the 11-key standalone piano from the Seed proto.
- Not a Daisy **product** and not a Pi/CM5 project. The old Seed box is a temporary M1 testbed only. See [testbed.md](testbed.md).
- Not USB MIDI host (no “plug a controller into M1 with no computer” as a v1 requirement).
- Not a settings app, a plugin UI, or the old OpenChord menu system.
- Not Orchid / ORC-1. Orchid is a battery synth with speakers. MIDI in is a side door. M1 is the opposite: **great at the MIDI path**, no sound of its own.
- Not Kordbot. We are not shipping a $350 cockpit.

If a feature needs audio, a big screen, or a second computer, it belongs on **S1**, not a “quick add” to M1.

---

## Who it is for

First user is us: MIDI or live instruments into a DAW. Second user is anyone who wants Orchid-style chord control without buying a synth they will not use.

The keyboard (or the DAW piano roll) is the keyboard. M1 is the left hand and the brain.

---

## Interaction (v1 lock)

### Incoming MIDI is the root

USB (and later TRS) notes from the PC / keyboard choose the **root**. This device is a USB MIDI **device** on the box USB-C: PC sends notes in, chords come back out to the DAW.

No mini-piano on M1. Later we can try “smart Orchid” (type buttons follow the note / key). Not I–vii on these eight keys — that was the OG. See [chord-engine.md](chord-engine.md).

### Buttons are Orchid + menu

Eight chord keys, Orchid grammar. Combinable. **Maj means maj.** Menu/options on the extra proto keys (Key, Shift). Dynamic-from-note is a later experiment, not the v0 proto.

| Type | Extension |
|------|-----------|
| Dim, Min, Maj, Sus | 6, m7, M7, 9 |

OG I–vii is history. The stick is not a quality table.

### Stick is color, not type

Alps **RKJXV1220001** (LCSC **C219778**). Analog, 8 seats in firmware. **Spring-back to center.** Center = HOME. Lean is temporary.

Seats: voicing / bass / **in-key** color. Not maj vs min. Not the archive preset table. Out of key only if the player tries (Orchid maj on a minor degree, spice later).

```
        more tension
   7         8         9
inv  4     HOME      6   inv
   1         2         3
        tighter / lower
```

That diagram is a **candidate**, not a lock. Seats are voicings (or extra color), never chord types. HiChord-style quality roulette is a lab mode at most.

No encoder on v1. No 8-way HVAC switch. A D-pad is a later maybe; the proto already has analog.

Split: **buttons pick the chord (Orchid), stick colors it (in key).** 2 tactiles = Key + Shift.

The algorithm:

- Remembers the last chord and voice-leads.
- Stick extras stay in key. Orchid type can leave the key on purpose (maj means maj).
- Stick never changes type.
- Key override: Key hold + incoming MIDI note.

Optional later: a dedicated “spice” gesture (stick click or hard edge) for one borrowed chord from a mood table. Spice is a door the player opens, not the miss penalty.

### Display and settings

We want as little screen as we can get away with. We still need to know **key** and **current chord**.

- Prefer a tiny OLED that only shows something like `Cmaj7` and `C major`.
- Or four LEDs plus that. A $2 OLED is not a jambox.
- Key override: hold **Key** + incoming note.
- Shift: thru / replace / panic and anything that is not a chord button.
- No companion app required for v1.
- No OpenChord-style hierarchical menu.

Zero screen is allowed only if we accept “what key am I in?” as the first support question. Default plan is the tiny OLED.

---

## Hardware (v1)

| Piece | Plan |
|-------|------|
| MCU | **RP2040** on our PCB (TinyUSB MIDI device, UART TRS). Not ESP32. Not a Pico glued to a carrier. |
| USB | USB-C on the **edge of our board**, device only, also power |
| MIDI | TRS in, TRS out; USB MIDI in/out |
| Front | 8× Gateron low-profile (type + extension), Alps RKJXV1220001, 2 cheap tactiles (Key, Shift) |
| Display | Cheap 0.91" I2C SSD1306 from LCSC, not Adafruit |
| Audio | None |
| Battery | None |
| Host USB | None |
| Price | Under **$100** to the customer, with margin, after the first messy run |

One custom PCB. SMT chip + flash + crystal + USB-C. Through-hole for stick, TRS, switches as needed. Reproducible. No breakout nest.

**Prove the engine before that board.** Portable logic first. Hands-on feel on the **RP2040-Zero proto** (`m1/proto-rp2040`) with a Launchkey. Seed box is parked on archived OG firmware. See [testbed.md](testbed.md).

---

## Chord engine — what we are fixing

The archived Seed mapper (`archive/s1-daisy` chord engine + joystick presets) had the right *idea* and a bad *feel*:

- Stick **replaced** quality instead of coloring the diatonic chord.
- Eight unique qualities, nastier ones on diagonals, analog stick cannot hit eight slices.
- No memory of the previous chord, no voice leading. Closed root-position stacks every time.
- Presets were the same eight qualities shuffled.

M1’s engine should make it **almost impossible to sound bad** and **still surprising**, then get good at **progressions**. That is the product. Do not port the old preset tables as the v1 model. Design it in [chord-engine.md](chord-engine.md) — that file is next.

Useful scraps in the archive: scale/mode tables, interval lists, enclosure photos, “chord as an input plugin” as a concept. Not the 8-way quality map.

---

## Positioning

| Device | Why we are not that |
|--------|---------------------|
| Orchid ORC-1 | Instrument with speakers and battery. We are a studio MIDI tool. |
| Kordbot | Huge, expensive, 32 buttons. We stay small. |
| Scaler / Captain / ChordAXE | Software. We are hardware in the hands. |
| Old OpenChord proto | Jambox. That is S1. |

Sell path: a board we can spin and price **under $100**, not a synth price. MCU and radio were never going to be the margin. Enclosure, assembly, and not certifying Wi‑Fi are.

---

## Repo and process

- Family lives in this repo (`OpenChord`). No product branches.
- M1 work stays under `m1/`.
- `s1/` stays empty until the jambox is a real project. **Do not dump M1 proto firmware there.**
- `archive/s1-daisy/` is frozen. Read it. Do not edit it. Restore the OG box by rebuilding that tree.
- Seed proto pins follow the archive **driver Init()** functions, not `pin_config.h` or `docs/hardware/pinout.md`. See [testbed.md](testbed.md) and `m1/proto-daisy/pin_map.h`.

```
m1/engine/          portable chord logic (no Daisy, no TinyUSB). This is the product.
m1/proto-rp2040/    RP2040-Zero lab harness. USB-C MIDI device. Launchkey pads/knobs.
m1/proto-daisy/     parked Seed harness. Restore OG from archive; do not edit archive.
m1/firmware/        RP2040 product firmware (later)
m1/hardware/        PCB / enclosure / KiCad libs
m1/docs/            this file, testbed, chord-engine
```

Build order:

1. Spec (this doc) — in progress.
2. Portable engine (`m1/engine`).
3. Play it on the RP2040-Zero proto (`m1/proto-rp2040`).
4. Iterate engine + proto until C–Am–F–G never sounds stupid.
5. PCB: RP2040, USB-C on our edge, TRS, Gaterons, Alps, OLED.
6. Enclosure last.

---

## Open decisions (on purpose)

These are not forgotten. They are not locked.

- MIDI channel split (chords / bass / thru) — Orchid does this; we may want it.
- Thru vs replace vs merge when a keyboard already sends chords.
- Stick click for spice vs Shift-only.
- Stick map (voicing vs leftover HiChord-style quality). Play it; do not lock in a meeting.
- One-hand layout: 2×4 cluster + thumb stick vs stick-only vs buttons-only.
- How much “next chord” suggestion is v1 vs v1.1.
- Trademark / `openchord.com` is someone else’s music-apps site. Product name is still OpenChord M1; do not assume the domain.

Locked (do not reopen without updating this file): MCU RP2040; custom PCB with USB-C on the edge; no radio; Alps C219778 quantized to 8; 8 Gaterons + Key/Shift; cheap OLED; ~$99; engine portable; RP2040-Zero as current testbed; Seed proto parked; archive frozen.

When one of these is decided, update this file. Do not start a second source of truth.
