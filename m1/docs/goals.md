# OpenChord M1 — goals and plan

Written September 2026 so we do not lose the decisions that led here. This is the product spec for v1, not firmware.

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

---

## What M1 is not

- Not a synth, sampler, looper, or drum machine.
- Not the 11-key standalone piano from the Seed proto.
- Not a Daisy project and not a Pi/CM5 project.
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

Notes from TRS in or USB in choose the root (or the line being transformed). We do not put a chromatic mini-keyboard on the box for v1.

If we later want “no keyboard on the desk,” extra keys should be **scale degrees** (I–vii), not a tiny piano. That is v1.1 or a second SKU. Do not design the first PCB around it.

### Buttons are the dictionary (hardcoded, trustworthy)

Steal Orchid’s grammar. It is already learned.

| Row | Buttons |
|-----|---------|
| Type | Dim, Min, Maj, Sus |
| Extension | 6, m7, M7, 9 |

Hold type + extension. Combinable. **Maj means maj.** The Seed joystick failed because it replaced diatonic function with a quality roulette (ii in C becoming Dmaj9, dim/aug on missed diagonals).

Buttons never get clever. They are the reliable layer.

### One fluid control is the algorithm

Either:

- **Encoder (safer to sell):** voicing / spread / inversion.
- **Stick (OpenChord DNA):** X = bass / inversion (wide zones: root, 1st, 2nd). Y = tension 0–1 through a scale-aware table. Diagonals mix both. They are not a third identity. No eight pie slices.

Split: **buttons = chord identity, stick or knob = color and voice leading.**

The algorithm:

- Remembers the last chord.
- Voice-leads every change (common tones stay, other notes take the shortest path).
- Stays in the current key unless a button forces borrowed color.
- Picks among *legal* voicings so it stays surprising without sounding stupid. Surprise is “which good voicing,” not random diminished.
- Key = auto from what you play, override with a Key hold + a note from the incoming keyboard.

Optional later: a dedicated “spice” gesture (stick click or hard edge) for one borrowed chord from a mood table. Spice is a door the player opens, not the miss penalty.

### Display and settings

We want as little screen as we can get away with. We still need to know **key** and **current chord**.

- Prefer a tiny OLED that only shows something like `Cmaj7` and `C major`.
- Or four LEDs plus that. A $2 OLED is not a jambox.
- Key override: hold Key + incoming note.
- Deeper settings: long-press + the voicing knob, or MIDI CC.
- No companion app required for v1.
- No OpenChord-style hierarchical menu.

Zero screen is allowed only if we accept “what key am I in?” as the first support question. Default plan is the tiny OLED.

---

## Hardware (v1)

| Piece | Plan |
|-------|------|
| MCU | ESP32-S3 (USB MIDI device via TinyUSB, UART for TRS MIDI) |
| USB | USB-C, device only, also power |
| MIDI | TRS in, TRS out; USB MIDI in/out |
| Front | 8 chord buttons + 1 voicing encoder and/or analog stick |
| Display | Optional 0.91" I2C OLED |
| Audio | None |
| Battery | None |
| Host USB | None |

Custom PCB. Reproducible. No Adafruit-breakout nest.

**Prove the brain before the board.** An S3 devkit (or even a Pi 5) plus a MIDI keyboard is enough to know if “I played C–Am–F–G and it voiced itself and never sounded stupid” is real. If that is not magic, the enclosure will not save it.

---

## Chord engine — what we are fixing

The archived Seed mapper (`archive/s1-daisy` chord engine + joystick presets) had the right *idea* and a bad *feel*:

- Stick **replaced** quality instead of coloring the diatonic chord.
- Eight unique qualities, nastier ones on diagonals, analog stick cannot hit eight slices.
- No memory of the previous chord, no voice leading. Closed root-position stacks every time.
- Presets were the same eight qualities shuffled.

M1’s engine should make it **almost impossible to sound bad** and **still surprising**. That is the product. Do not port the old preset tables as the v1 model.

Useful scraps in the archive: scale/mode tables, interval lists, enclosure photos, “chord as an input plugin” as a concept. Not the 8-way quality map.

---

## Positioning

| Device | Why we are not that |
|--------|---------------------|
| Orchid ORC-1 | Instrument with speakers and battery. We are a studio MIDI tool. |
| Kordbot | Huge, expensive, 32 buttons. We stay small. |
| Scaler / Captain / ChordAXE | Software. We are hardware in the hands. |
| Old OpenChord proto | Jambox. That is S1. |

Sell path we liked: a board we can spin, assemble, and price in the studio-tool range (thinking ~$149–199), not a synth price.

---

## Repo and process

- Family lives in this repo (`OpenChord`). No product branches.
- M1 work stays under `m1/`.
- `s1/` stays empty until the jambox is a real project.
- Daisy proto stays in `archive/s1-daisy/`.

Build order:

1. Spec (this doc).
2. Chord engine on a bench (devkit + MIDI keyboard).
3. PCB: S3, USB-C, TRS, buttons, voicing control, optional OLED.
4. Enclosure last.

---

## Open decisions (on purpose)

These are not forgotten. They are not locked.

- Encoder vs stick vs both on the first PCB.
- Exact OLED vs LED-only.
- Whether “smart amount” is the same physical control as voicing.
- MIDI channel split (chords / bass / thru) — Orchid does this; we may want it.
- Thru vs replace vs merge behavior when a keyboard already sends chords.
- Trademark / `openchord.com` is someone else’s music-apps site. Product name is still OpenChord M1; do not assume the domain.

When one of these is decided, update this file. Do not start a second source of truth.
