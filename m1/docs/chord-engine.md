# Chord engine

Portable logic for OpenChord M1. Code: `m1/engine`. Product: `goals.md`. Hands-on: `testbed.md`.

Stick **gesture** is spring-back to HOME. Stick **must not** be a static quality map. Buttons are Orchid types. Feel lock: fun, stick extras in key, jazz/wrong only if the player *tries* (honest Maj, spice later).

Do not port the archive joystick preset tables.

---

## Split

| Layer | Who | Clever? |
|-------|-----|--------|
| **Which chord** | Incoming MIDI = root. Buttons = Orchid type + extras | Maj means maj. Out of key is a choice. |
| **How it sits** | Stick (lean, spring home) | Voicing, bass, in-key color. **Never** maj vs min. |
| **Glue** | Key + last chord + voice leading | Always on. |

Stick and buttons must not do the same job.

---

## Where this came from (correction)

OG OpenChord **was** HiChord-shaped. The analysis that “we forgot I–vii” was wrong.

- **Buttons 1–7 = I–vii of the key you picked.** Not chromatic. In C, button 2 is the ii chord, which *should* be minor.
- **Stick = 8 hardcoded quality slices** (Classic / Jazzy / Ambient in archive `chord_engine.cpp`). Same 8 qualities every time. Hold + lean, release snaps back — same gesture as HiChord.
- Almost no voice leading.

The failure was the **static stick**, not missing diatonic buttons. Even with I–vii in key, the stick could turn ii into Dmaj9 / aug / dim. **About half the seats were out of key** for that degree. Fine once if you meant it. Trash as the default: you could not rest on the stick without leaving the key.

M1 **does not** put I–vii on these eight keys. That was the OG. The new box is Orchid grammar + MIDI root. Later we can try **smart Orchid** (types follow the played note / key). Figure that out later.

**Stick only colors the chord you already have.** Extra notes stay in key unless the player opens a door (Orchid maj on a ii, spice later).

| | HiChord | OG Seed | M1 proto |
|--|--|--|--|
| Buttons | I–vii | I–vii | **Orchid:** Dim / Min / Maj / Sus + 6 / m7 / M7 / 9. Extra keys = Key / Shift |
| Root | Degree of the key | Degree of the key | **Incoming USB MIDI note** |
| Stick | 8 quality transforms, snap back | Same, static presets | Snap back, **voicing / bass / in-key color** |
| Out of key | Easy (half the stick) | Easy (half the stick) | Stick no. Type yes if you try |

---

## Priority (feel, not every knob)

1. Fun to play.
2. **Always sounds good** on the default path (in key, voice-led HOME).
3. **Not stuck boring.** Jazzy / borrowed / “wrong” when the player *tries* (spice, type override, extra tensions). Never as the penalty for a missed slice or for holding the stick.

Center + the type you held are the safe road. Interesting is a lean or a held extra.

---

## Buttons — Orchid (locked for this proto)

Same 8 Gaterons. They are **not** I–vii.

| Bottom | Dim | Min | Maj | Sus |
| Middle | 6 | m7 | M7 | — |
| Top | 9 | Key | Shift | panic |

Incoming MIDI note is the **root**. Type buttons combine with extras. **Maj means maj** even if the key wanted minor. That is a legal way to *try* to go out. Stick still does not change type.

Extra proto keys are menu/options: **Key** (hold + note sets the key), **Shift** (reserved), RECORD = panic.

Later experiment, not this firmware: **smart Orchid** — the same type cluster, but qualities follow the note / key so you can mash Maj and still land in-key. Do not put I–vii on these eight keys to get that.

---

## Hands

Box left of a keyboard. Left hand is M1.

1. **Buttons only** — complete at HOME. Stick optional.
2. **Buttons + thumb stick** — 2×4 cluster, stick on the inner edge where the thumb lives. Lean = color, release = HOME.
3. **Stick-only** — needs latched last degree / type.

Hold vs latch on the **buttons** is still open. This proto **assigns at note-on**: the type held when a key goes down sticks to that key until it comes up. Releasing the pad does not drop the chord. New keys with no pad are thru.

---

## Stick — contract

Hardware: analog, snap to 8 seats + center.

**Locked:** spring-back. Center = HOME. Lean is temporary. Release = HOME. Not sticky.

**Locked:** seats are not chord types. No static 8-quality table, even if buttons are I–vii. That was the trash part.

The stick should be:

1. **Dynamic** — notes depend on current chord, key, and last HOME (voice leading). Same seat, different MIDI next bar.
2. **Predictable** — same gesture, same *kind* of thing.
3. **In key by default** — extra notes from the scale. ♯11 / dim-for-fun = spice, not a diagonal. Maj on a ii is the type button, not the stick.
4. **Fun / not boring** — up should still do something you can hear (open voicing and/or in-key 9/11/13).

### Predictability (voicing stick)

- **Up** — more color / more open / more (in-key) tension. Never “become major.”
- **Down** — tighter / closer. Never “become minor.”
- **Left / right** — bass / inversion.
- **Center / release** — HOME.
- **Corners** — both axes, not a third identity.

If a tester cannot say what up does in one sentence, too clever.

### What the stick is not

- Not 8 chord qualities. Archive failure even with I–vii buttons.
- Not a progression picker.
- Not a key picker.
- Not sticky.

Lab “quality stick” mode: only as a compile flag to A/B against this. Not the default.

---

## Stick — candidate map (try this)

**Role + context.** Seats are roles. Notes are picked smart:

Given: current chord (from buttons) + key + last HOME + seat  
Pick pitches that (1) keep that chord’s identity, (2) match the role, (3) voice-lead, (4) stay in key unless spice is open.

Example, C major, MIDI D + Min held → Dm.

- HOME: D–F–A near the last chord.
- RIGHT: F in the bass, rest voice-led.
- UP: add in-key 9 (E) and/or open the spacing.
- Release: back to HOME, still Dm.

Same lean with MIDI G + Maj does that job to G, not “the same MIDI notes.”

Start proto here. If it feels dead, enrich the up-ladder (still in key) before bringing back quality slices.

---

## Tension ladder (up / down)

The type cluster already has 6 / m7 / M7 / 9, so up is (1) spacing and/or (2) unnamed in-key extras the buttons did not already hold. Held 9 always wins.

Never add ♯11 / ♭9 from a normal seat. That is spice.

Down: reverse extras, then close the stack. Never change type.

---

## Inversion / bass (left / right)

- HOME: root in the bass.
- One step: 3rd in bass.
- Two steps: 5th.
- Further: 7th in bass if a 7th is already in the chord.

Non-inversion slashes (C/D) = spice. Do not hide them in a normal seat.

---

## HOME

Stick centered or just released:

- Default chord. Buttons-only players live here.
- The chord the buttons asked for, voice-led. Buttons-only at HOME must already sound like a product.
- Voice-lead from the previous HOME. Not a fresh closed pile every time.
- Top note biased into a useful band (around G3–G5).

If HOME is ugly, stop. The stick cannot save it. If HOME is great, the stick is gravy.

---

## Voice leading (always on)

1. Keep common tones.
2. Other voices, shortest path.
3. Bass vs top: prefer contrary or oblique.
4. No voice crossing unless the seat is “open.”
5. If the only legal voicing leaps, leap rather than a wrong note.

---

## Key

- Buttons ignore key for quality (Orchid is honest). Stick extras and spice still read key.
- Override: hold **Key** + incoming MIDI note.
- Display: `Dm7` and `C major`.

Wrong key should only make stick extras weird, not change Min into Maj.

---

## Spice (the “I tried” door)

Stick click, Shift+stick, or a labeled edge. Borrowed color, secondary dominant, modal mixture. Player opened it. Diagonals are not spice. Half-the-stick-out-of-key is not spice.

v0 can ship spice = off and still not be boring if the in-key up-ladder is audible.

---

## MIDI (this proto)

Lab box: RP2040-Zero USB-C as MIDI **device** `OpenChord M1`. Launchkey Mini MK4 keys = roots. Pads = Orchid types (ch 10 CC 36–43). Two knobs = stick (CC 47/48).

Each piano key is assigned at **note-on**:

- Type pad held → that key is a chord (root not echoed). Stays until **that** key is released, even if the pad is let go.
- No type pad → **thru**.
- A new key does not revoice an already-held chord. Two type pads can hold two chords at once.

PC is USB host. Launchkey → DAW/router → OpenChord in. OpenChord out → instrument track. Do not also send Launchkey notes straight to the instrument.

Outgoing notes are ref-counted so a thru E on top of C major does not kill the chord’s E. Channel split later.

---

## v0 playtest

1. USB MIDI in = root. Hold Dim/Min/Maj/Sus (+ extras) and play a key. Chord stays until that key is up.
2. Stick roles, spring-back, **in-key extras only**. Type never comes from the stick.
3. Key override (Key + note). OLED: chord + key. (Product; not on the Zero proto yet.)
4. Thru when no type is held at note-on.
5. Smart Orchid / I–vii later if this is boring or too easy to leave the key.

Do not add quality slices on the stick. That is the OG.

---

## Open questions

Locked: spring-back HOME. Stick ≠ static quality map. Stick extras in key. Out of key is the type you held (or spice later).

Decided for this proto: Orchid buttons + MIDI root. Not I–vii on the eight keys.

Still open:

1. Smart Orchid (types follow the note/key) — later.
2. HOME = triad or always add a 7th?
3. Up: spacing, in-key extras, or both?
4. Keep role while leaning across a new type (probably yes)?
5. Hold vs latch on the physical buttons? Note-on assignment is what the RP2040 proto does.
6. Layout: thumb-reach stick?

When one is decided, update this file.
