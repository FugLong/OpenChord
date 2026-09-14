# Chord engine

Portable logic for OpenChord M1 / M Core. Code: `m1/engine`. Product: `goals.md`. Hands-on: `testbed.md`. Hosts: RP2040 firmware **and** the free plugin. Neither host may fork the voicing math.

Stick **gesture** is temporary color then back to HOME (lift finger on the cap wheel, or spring-back on Alps fallback). Stick **must not** be a static quality map — not in Pro mode, not in Smart mode. Feel lock: fun, stick extras in key, spicy / out-of-key only if the player *tries*.

Do not port the archive joystick preset tables.

UI mode names: **Pro** and **Smart**. Do not use competitor product names in UI, docs, or code comments.

---

## Two audiences, two modes

| Mode | Who | What you need | What the 8 pads do |
|------|-----|---------------|--------------------|
| **Pro** | Keyboard players, musicians who want full control | MIDI keyboard (or piano roll) for roots | Dim / Min / Maj / Sus + extras 6 / m7 / M7 / 9 |
| **Smart** | Bedroom producers, travel, phone + box, no big keyboard | Just the pads (+ stick). Keyboard optional | I–vii of the key, 8th pad = **high I** |

**Pro** = advanced left-hand chord grammar on top of a real keyboard. Honest: Maj means maj even if the key “wanted” minor.

**Smart** = always-sounds-good diatonic pads. Hold a pad to hold the chord (two-handed on the box: one hand stick, one hand degrees). HOME is a **triad**. Optional later settings can add fancy diatonic 7ths for beginners who want richer defaults — not the v0 default.

---

## Split

| Layer | Pro mode | Smart mode | Clever? |
|-------|--------------|------------|---------|
| **Which chord** | Incoming MIDI = root. Pads = triad + extras | Pads = I–vii (+ high I). Diatonic quality from the key at HOME | Pro: Maj means maj. Smart: HOME is diatonic. Out of key is a choice. |
| **How it sits** | Stick (rim seat / lean, then HOME) | Same stick contract | Voicing, bass, in-key color. **Never** maj vs min. |
| **Glue** | Key + last chord + voice leading | Same | Always on. |

Stick and pads must not do the same job.

---

## Where this came from (correction)

The early Seed mapper put scale degrees on buttons but used an **8-way static quality stick**. Same eight transforms every time. About half the seats left the key for a given degree — lean on ii and you get a major-ish mess. Gesture was fine; the **static stick** was the failure.

M1 / M Core:

- **Pro** — MIDI note is the root; pads are quality + extras (current Launchkey proto + plugin default).
- **Smart** — pads are I–vii + high I; stick only colors; HOME is diatonic triad; hold-to-sound.

**Stick only colors the chord you already have.** Extra notes stay in key unless the player opens a door (Pro-mode Maj on a minor degree, spice later).

| | Early Seed stick map | Pro | Smart |
|--|--|--|--|
| Pads | I–vii | Dim / Min / Maj / Sus + extras | I–vii + high I |
| Root | Degree of the key | Incoming MIDI note | Degree of the key (keyboard optional) |
| Stick | 8 quality transforms | Snap back to HOME, voicing / bass / in-key color | Same coloring stick. Not quality slices. |
| Out of key | Easy (half the stick) | Stick no. Pro pad yes if you try | Stick no. Override / spice if you try |
| Sound | Old jambox synth | None (MIDI out) | None (MIDI out) |

---

## Priority (feel, not every knob)

1. Fun to play.
2. **Always sounds good** on the default path (in key, voice-led HOME).
3. **Not stuck boring.** Jazzy / borrowed / “wrong” when the player *tries* (spice, quality override, extra tensions). Never as the penalty for a missed stick slice.

Center + the chord the mode asked for are the safe road. Interesting is a lean or a held extra.

---

## Pads — Pro (locked)

| Bottom | Dim | Min | Maj | Sus |
| Middle | 6 | m7 | M7 | — |
| Top | 9 | Key | Shift | panic |

Incoming MIDI note is the **root**. Pro pads combine with extras. **Maj means maj** even if the key wanted minor.

Assignment at **note-on** (Pro): type held when a key goes down sticks to that key until that key comes up. No type → thru.

Later experiment: **auto chord-type** (qualities follow the note/key). Do not confuse with **Smart** mode.

---

## Pads — Scale (locked for product)

Same 8 pads, different map:

| Pad | Degree |
|-----|--------|
| 1–7 | I, ii, iii, IV, V, vi, vii° (major-key diatonic triad at HOME) |
| 8 | **High I** (I one octave up) |

**Hold** while pressed — chord on while held, off on release. No keyboard required. Designed for two hands on the box (stick + degrees).

HOME = **triad only** by default. Enhancement settings later (auto diatonic 7ths, etc.) for richer beginner defaults — off unless chosen.

Keyboard in Smart: optional melody thru; **Key + note** still sets the key. Stick contract identical to Pro: color, not quality. ii + up is more open / in-key color on Dm, not a major transform.

---

## Hands

Box left of a keyboard (Pro) or alone (Smart).

1. **Pads only** — complete at HOME. Stick optional. Pro needs a root from MIDI; Smart does not.
2. **Pads + thumb stick** — 2×4 cluster, stick (cap wheel plan) on the inner edge. Lean / rim = color, release = HOME.
3. **Stick-only** — needs latched last degree / quality (later).

---

## Stick — contract

Hardware: analog, snap to 8 seats + center.

**Locked:** spring-back. Center = HOME. Lean is temporary.

**Locked:** seats are not chord types. No static 8-quality table in either mode.

1. **Dynamic** — notes depend on current chord, key, and last HOME (voice leading).
2. **Predictable** — same gesture, same *kind* of thing.
3. **In key by default** — extra notes from the scale.
4. **Fun** — up should be audible (open voicing and/or in-key 9/11/13).

### Predictability

- **Up** — more color / more open. Never “become major.”
- **Down** — tighter / closer. Never “become minor.”
- **Left / right** — bass / inversion.
- **Center / release** — HOME.
- **Corners** — both axes.

### What the stick is not

- Not 8 chord qualities.
- Not a progression picker.
- Not a key picker.
- Not sticky.

---

## Stick — candidate map

Given: current chord + last HOME + seat → pitches that keep identity, match the role, voice-lead, stay in key.

Smart mode, C major, pad ii: Dm triad at HOME; up adds in-key color; release still Dm.

---

## Tension ladder (up / down)

Pro mode already has 6 / m7 / M7 / 9 pads, so up is spacing and/or unnamed in-key extras.

Smart mode has no extra pads at first — the up-ladder *is* how you add in-key color.

Down: reverse extras, then close. Never change type.

---

## Inversion / bass (left / right)

- HOME: root in the bass.
- One step: 3rd in bass.
- Two steps: 5th.
- Further: 7th in bass if a 7th is already in the chord.

---

## HOME

- Default chord. Pads-only players live here.
- Voice-lead from the previous HOME.
- Follow played octave (Pro) or a sensible default register (Smart, ~C3 tonic area).

If HOME is ugly, stop. The stick cannot save it.

---

## Voice leading (always on)

1. Keep common tones.
2. Other voices, shortest path.
3. Bass vs top: prefer contrary or oblique.
4. No voice crossing unless the seat is “open.”

---

## Key

- Pro: pads ignore key for quality (honest Maj). Stick extras still read key.
- Scale: key **is** the map.
- Override: hold **Key** + incoming MIDI note.
- Display: chord name, key name, Pro/Smart.

---

## Spice (the “I tried” door)

Later: hard rim edge / Shift for borrowed color. Player opened it. Diagonals are not spice.

v0 can ship spice = off if the in-key up-ladder is audible.

---

## MIDI (Pro proto / plugin)

Lab box: RP2040-Zero USB-C as MIDI **device** `OpenChord M1`. Launchkey pads CC 36–43, stick CC 47/48.

Plugin: same engine; Learn remaps; mode toggle Pro ↔ Smart. Same eight pad bindings; meaning follows mode.

---

## Open questions

Locked now:

- UI names **Pro** and **Smart**.
- Stick ≠ static quality map in both modes.
- Smart 8th pad = high I.
- Smart HOME = triad; fancy 7ths = later setting.
- Smart pads = **hold** (not latch).
- No competitor product names in UI / docs / code comments.

Still open:

1. smart Pro (types follow the note/key) — later.
2. Up: spacing, in-key extras, or both? (try both)
3. Keep role while leaning across a new chord (probably yes)?
4. Layout: thumb-reach stick?
5. Smart enhancement presets (auto-7ths, etc.) — which first?

When one is decided, update this file.
