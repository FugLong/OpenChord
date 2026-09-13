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

So: a smaller, MIDI-only product that can live on a custom PCB and actually be sold, plus a free plugin that runs the same engine. S1 stays in the family for later.

The Daisy tree is archived at [`../../archive/s1-daisy/`](../../archive/s1-daisy/). Do not treat it as S1 source.

---

## What M1 is

A **man-in-the-middle MIDI enhancer**. Feed it MIDI from a real keyboard, a controller, or the DAW. It sends MIDI back transformed: chords, voicings, extensions, voice leading.

Two hosts, **one engine** (`m1/engine`):

- **Hardware** — RP2040 box, USB-C MIDI device, later TRS. The thing you buy.
- **Plugin** — free, MIT, AU MIDI FX + VST3. Same `oc::Render()`. MIDI Learn so any pads/stick/knobs can be the cluster.

- No audio (neither host).
- No USB host on the box. The PC is the USB host. M1 hardware is a USB MIDI **device**.
- Hardware: TRS MIDI in and TRS MIDI out (plus USB MIDI in/out).
- Bus-powered from USB-C. **No battery** on v1. If TRS-only gear needs it, plug USB-C into power with no data.
- Small enough to sit on a desk next to a keyboard and disappear into a session.

One-line pitch: *sits between your keys and the DAW and makes you sound like you know theory.*

Ambition: the most helpful chord/MIDI tool we have used. Not a quality roulette. It should voice well, stay in key, and later help with **where to go next** (progressions, ideas). Voicing that never sounds stupid is v1. Progression-smart is the north star; do not fake it with random borrowed chords. Write the model in [chord-engine.md](chord-engine.md).

---

## What M1 is not

- Not a synth, sampler, looper, or drum machine.
- Not the 11-key standalone piano from the Seed proto.
- Not a Daisy **product** and not a Pi/CM5 project. The old Seed box is a temporary M1 testbed only. See [testbed.md](testbed.md).
- Not USB MIDI host (no “plug a controller into M1 with no computer” as a v1 requirement).
- Not the old OpenChord hierarchical menu on the box. Extra settings live in the **plugin**, not extra knobs on the enclosure.
- Not Orchid / ORC-1. Orchid is a battery synth with speakers. MIDI in is a side door. Type-mode buttons are ordinary chord grammar (triad + extras), not their product. M1 is the opposite of Orchid: **great at the MIDI path**, no sound of its own.
- Not HiChord. They are a synth with a specific slab. **Degree** mode takes the I–vii idea and voices it properly (coloring stick, not eight frozen qualities). Do not use their name, enclosure, or pad art in the UI or marketing.
- Not Kordbot. We are not shipping a $350 cockpit.

If a feature needs audio or a big on-device screen, it belongs on **S1**, not a “quick add” to M1. If it is a MIDI preference (channel split, voicing tightness, maps), it belongs in the plugin.

---

## Who it is for

First user is us: MIDI or live instruments into a DAW. Second user is anyone who wants left-hand chord control without buying a synth they will not use. The plugin is how people try it with a Launchkey (or anything). The box is for people who want that control in their hands.

The keyboard (or the DAW piano roll) is the keyboard in Type mode. In Degree mode the eight keys *are* the degrees and a keyboard is optional. Either way M1 is the left hand and the brain.

---

## Interaction (v1 lock)

Two **modes**. Same eight Gaterons, different job. Toggle with a tiny hardware button (or Shift+hold). OLED and plugin both show which mode you are in. Names in the UI: **Type** and **Degree**. Not “Orchid” or “HiChord.”

### Type mode — incoming MIDI is the root

USB (and later TRS) notes from the PC / keyboard choose the **root**. Eight buttons are triad + extras. Combinable. **Maj means maj.**

| Type | Extension |
|------|-----------|
| Dim, Min, Maj, Sus | 6, m7, M7, 9 |

This is the current RP2040-Zero proto (Launchkey keys = root, pads = types). Dynamic-from-note (“smart Type”) is a later experiment, not the v0 proto.

No mini-piano on M1.

### Degree mode — buttons are I–vii

The eight keys are degrees of the current key (I–vii, plus whatever the eighth does — likely I in another octave or a spare). In C, button 2 is ii, which *should* be minor at HOME. No keyboard required.

Default quality comes from the scale (ii min, V maj, vii dim). Stick still **colors** that chord (voicing / bass / in-key extras), same contract as Type mode. Stick is not eight frozen qualities. That was the OG failure: lean on ii and you get Dmaj9. Out of key is a Type override or spice, not a missed slice.

Degree mode is a **product mode**, not this proto’s job. Spec it in [chord-engine.md](chord-engine.md). Play Type until HOME is great, then add Degree.

### Stick is color, not type

Alps **RKJXV1220001** (LCSC **C219778**). Analog, 8 seats in firmware. **Spring-back to center.** Center = HOME. Lean is temporary.

Seats: voicing / bass / **in-key** color. Not maj vs min. Not the archive preset table. Both modes. Out of key only if the player tries (Type-mode Maj on a minor degree, spice later).

```
        more tension
   7         8         9
inv  4     HOME      6   inv
   1         2         3
        tighter / lower
```

That diagram is a **candidate**, not a lock. Seats are voicings (or extra color), never chord types. No quality-roulette stick, including in Degree mode.

No encoder on v1. **No pots / dials on the box.** They add size and cost, and this product assumes a DAW. A D-pad is a later maybe; the proto already has analog.

Split: **buttons pick the chord, stick colors it (in key).** Tactiles = Key, Shift, and a mode toggle (exact combo still open). Panic can stay a chord-row extra or Shift.

The algorithm:

- Remembers the last chord and voice-leads.
- Stick extras stay in key. Type-mode quality can leave the key on purpose (maj means maj). Degree-mode HOME stays diatonic.
- Stick never changes type.
- Key override: Key hold + incoming MIDI note.

Optional later: a dedicated “spice” gesture (stick click or hard edge) for one borrowed chord from a mood table. Spice is a door the player opens, not the miss penalty.

### Display and settings

We want as little screen as we can get away with. We still need to know **key**, **current chord**, and **mode**.

- Prefer a tiny OLED that only shows something like `Cmaj7`, `C major`, and Type/Degree.
- Or four LEDs plus that. A $2 OLED is not a jambox.
- Key override: hold **Key** + incoming note.
- Shift / mode tactiles: mode, thru / replace / panic — only what you need without opening a laptop.
- **No companion required.** The box plays with no plugin. The plugin is how you bind controllers, see extra settings, and (when a box is plugged in) edit the device.
- No OpenChord-style hierarchical menu on the hardware.

Zero screen is allowed only if we accept “what key am I in?” as the first support question. Default plan is the tiny OLED.

---

## Hardware (v1)

| Piece | Plan |
|-------|------|
| MCU | **RP2040** on our PCB (TinyUSB MIDI device, UART TRS). Not ESP32. Not a Pico glued to a carrier. |
| USB | USB-C on the **edge of our board**, device only, also power |
| MIDI | TRS in, TRS out; USB MIDI in/out |
| Front | 8× Gateron low-profile, Alps RKJXV1220001, a few cheap tactiles (Key, Shift, mode). **No pots.** |
| Display | Cheap 0.91" I2C SSD1306 from LCSC, not Adafruit |
| Audio | None |
| Battery | None |
| Host USB | None |
| Price | Under **$100** to the customer, with margin, after the first messy run |

One custom PCB. SMT chip + flash + crystal + USB-C. Through-hole for stick, TRS, switches as needed. Reproducible. No breakout nest.

**Prove the engine before that board.** Portable logic first. Hands-on feel on the **RP2040-Zero proto** (`m1/proto-rp2040`) with a Launchkey. Seed box is parked on archived OG firmware. See [testbed.md](testbed.md).

---

## Plugin (free)

Same product, second host. Code: later `m1/plugin/`. Engine stays `m1/engine`.

**Sell:** the box. **Give away:** the plugin, MIT, like the rest of the repo.

### Formats

Ship both from one project (JUCE is the practical stack):

| Format | Who |
|--------|-----|
| **AU MIDI FX** | Logic, GarageBand — real MIDI insert |
| **VST3** | Cubase, Reaper, Bitwig, Studio One as MIDI FX where the host allows it. Ableton / FL: instrument-style wrapper that **outputs MIDI** onto another track (Scaler-shaped routing). |
| Standalone | Optional; IAC / loopMIDI last resort |

Do not ship AU-only. Most DAWs cannot host an AU MIDI effect.

### Who runs the engine

| | No hardware | Hardware on USB as `OpenChord M1` |
|--|--|--|
| **Engine** | Plugin | Box |
| **Plugin** | MIDI FX + Learn + extra settings | Bridge / editor / live view. **Does not voice.** |

Never run two engines on the same notes. Doubled chords and fighting voice-leading.

When the box is connected, extra settings can push to the device over MIDI (SysEx is the boring, correct path). Firmware update can live here later.

### Mapping and UI

MIDI Learn on Type buttons, Degree buttons (same eight physical keys, two maps), Key, Shift, stick X/Y. Ship a Launchkey Mini MK4 preset that matches the current proto (pads CC 36–43, knobs CC 47/48).

v0 UI: chord name, key, mode, stick diagram, bindable pads. Ugly is fine.

Pretty overlay of the real enclosure is **later**, when the PCB exists. Do not skin the plugin as someone else’s device.

### Settings that are plugin knobs, not hardware dials

Examples, not a lock: channel split (chords / bass / thru), thru vs replace vs merge, voice-lead tightness, octave / range, smart vs honest Type, retrigger vs add-notes, stick map tweaks, Degree eighth-button job.

The box keeps defaults that already sound good. The plugin is the mixer.

---

## Chord engine — what we are fixing

The archived Seed mapper (`archive/s1-daisy` chord engine + joystick presets) had the right *idea* and a bad *feel*:

- Stick **replaced** quality instead of coloring the diatonic chord.
- Eight unique qualities, nastier ones on diagonals, analog stick cannot hit eight slices.
- No memory of the previous chord, no voice leading. Closed root-position stacks every time.
- Presets were the same eight qualities shuffled.

M1’s engine should make it **almost impossible to sound bad** and **still surprising**, then get good at **progressions**. That is the product. Do not port the old preset tables as the v1 model. Degree mode is how we take I–vii back without taking the static stick back. Design it in [chord-engine.md](chord-engine.md) — that file is next.

Useful scraps in the archive: scale/mode tables, interval lists, enclosure photos, “chord as an input plugin” as a concept. Not the 8-way quality map.

---

## Positioning

| Device | Why we are not that |
|--------|---------------------|
| Orchid ORC-1 | Instrument with speakers and battery. We are a studio MIDI tool. Type buttons are chord grammar, not their box. |
| HiChord | Synth + a specific slab. Degree mode is I–vii with our coloring stick and voice leading. |
| Kordbot | Huge, expensive, 32 buttons. We stay small. |
| Scaler / Captain / ChordAXE | Software-only, paid. We ship a **free** plugin and a **hardware** left hand. |
| Old OpenChord proto | Jambox. That is S1. |

Sell path: a board we can spin and price **under $100**, not a synth price. Plugin is free on purpose. MCU and radio were never going to be the margin. Enclosure, assembly, and not certifying Wi‑Fi are.

Name and domain: `openchord.com` is someone else’s music-apps site. Product name is still OpenChord M1; do not assume the domain. Do not put other products’ names on the hardware, plugin, or store page.

---

## Repo and process

- Family lives in this repo (`OpenChord`). No product branches.
- M1 work stays under `m1/`.
- `s1/` stays empty until the jambox is a real project. **Do not dump M1 proto firmware there.**
- `archive/s1-daisy/` is frozen. Read it. Do not edit it. Restore the OG box by rebuilding that tree.
- Seed proto pins follow the archive **driver Init()** functions, not `pin_config.h` or `docs/hardware/pinout.md`. See [testbed.md](testbed.md) and `m1/proto-daisy/pin_map.h`.

```
m1/engine/          portable chord logic (no Daisy, no TinyUSB, no JUCE). This is the product.
m1/proto-rp2040/    RP2040-Zero lab harness. USB-C MIDI device. Launchkey pads/knobs. Type mode.
m1/proto-daisy/     parked Seed harness. Restore OG from archive; do not edit archive.
m1/plugin/          AU MIDI FX + VST3 (later). Links m1/engine. Free.
m1/firmware/        RP2040 product firmware (later)
m1/hardware/        PCB / enclosure / KiCad libs
m1/docs/            this file, testbed, chord-engine
```

Build order:

1. Spec (this doc) — in progress.
2. Portable engine (`m1/engine`).
3. Play it on the RP2040-Zero proto (`m1/proto-rp2040`).
4. Iterate engine + proto until C–Am–F–G never sounds stupid.
5. Plugin that links the same engine — Learn, extra settings, ugly UI. Faster to iterate than flashing, and the free SKU.
6. PCB: RP2040, USB-C on our edge, TRS, Gaterons, Alps, OLED. No pots.
7. Enclosure last.
8. Pretty device GUI + SysEx editor once the hardware is real.

The plugin can start as soon as HOME on the proto is worth repeating. Do not wait for the custom board. Do not build the photoreal UI before the box exists.

---

## Open decisions (on purpose)

These are not forgotten. They are not locked.

- MIDI channel split (chords / bass / thru) — plugin setting; hardware default TBD.
- Thru vs replace vs merge when a keyboard already sends chords.
- Stick click for spice vs Shift-only.
- Exact tactiles: dedicated Mode vs Shift+hold. Panic placement.
- Degree mode: what the eighth button is (high I vs spare).
- One-hand layout: 2×4 cluster + thumb stick vs stick-only vs buttons-only.
- How much “next chord” suggestion is v1 vs v1.1.
- Trademark / `openchord.com` is someone else’s music-apps site.

Locked (do not reopen without updating this file): MCU RP2040; custom PCB with USB-C on the edge; no radio; no battery; no USB host on the SKU; Alps C219778 quantized to 8; 8 Gaterons + a few tactiles; **no pots / no encoder**; cheap OLED; ~$99 hardware; plugin free (AU MIDI FX + VST3, same engine); hardware works with no plugin; if the box is connected the plugin does not voice; Type + Degree modes (UI names); stick is color in both modes, never a quality table; engine portable; RP2040-Zero as current testbed; Seed proto parked; archive frozen.

When one of these is decided, update this file. Do not start a second source of truth.
