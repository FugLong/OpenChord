# Chord engine

Portable logic for OpenChord M1. Code: `m1/engine`. Product: `goals.md`. Hands-on: `testbed.md`. Hosts: RP2040 firmware **and** the free plugin. Neither host may fork the voicing math.

Stick **gesture** is spring-back to HOME. Stick **must not** be a static quality map — not in Type mode, not in Degree mode. Feel lock: fun, stick extras in key, jazz/wrong only if the player *tries* (honest Maj, spice later).

Do not port the archive joystick preset tables.

UI names: **Type** and **Degree**. Not Orchid. Not HiChord.

---

## Split

| Layer | Type mode | Degree mode | Clever? |
|-------|-----------|-------------|---------|
| **Which chord** | Incoming MIDI = root. Buttons = triad + extras | Buttons = I–vii of the key. Quality from the scale at HOME | Type: Maj means maj. Degree: HOME is diatonic. Out of key is a choice. |
| **How it sits** | Stick (lean, spring home) | Same stick contract | Voicing, bass, in-key color. **Never** maj vs min. |
| **Glue** | Key + last chord + voice leading | Same | Always on. |

Stick and buttons must not do the same job.

This proto and the first plugin pass are **Type**. Degree is specified here so we do not accidentally rebuild the OG stick when we add it.

---

## Where this came from (correction)

OG OpenChord **was** HiChord-shaped. The analysis that “we forgot I–vii” was wrong.

- **Buttons 1–7 = I–vii of the key you picked.** Not chromatic. In C, button 2 is the ii chord, which *should* be minor.
- **Stick = 8 hardcoded quality slices** (Classic / Jazzy / Ambient in archive `chord_engine.cpp`). Same 8 qualities every time. Hold + lean, release snaps back — same gesture as HiChord.
- Almost no voice leading.

The failure was the **static stick**, not missing diatonic buttons. Even with I–vii in key, the stick could turn ii into Dmaj9 / aug / dim. **About half the seats were out of key** for that degree. Fine once if you meant it. Trash as the default: you could not rest on the stick without leaving the key.

M1 **Type** mode does not put I–vii on these eight keys. Incoming note is the root; buttons are quality. That is the Launchkey proto.

M1 **Degree** mode *does* put I–vii on the eight keys. That is the OG / HiChord-like idea done properly: diatonic default, coloring stick, voice leading. It is a mode, not the only product, and it is not this proto.

**Stick only colors the chord you already have.** Extra notes stay in key unless the player opens a door (Type-mode Maj on a ii, spice later).

| | HiChord / OG Seed | M1 Type | M1 Degree |
|--|--|--|--|
| Buttons | I–vii | Dim / Min / Maj / Sus + 6 / m7 / M7 / 9 | I–vii of the key |
| Root | Degree of the key | Incoming MIDI note | Degree of the key (keyboard optional) |
| Stick | 8 quality transforms, snap back | Snap back, voicing / bass / in-key color | Same coloring stick. Not quality slices. |
| Out of key | Easy (half the stick) | Stick no. Type yes if you try | Stick no. Override / spice if you try |
| Sound | Their synth / our old jambox | None (MIDI out) | None (MIDI out) |

---

## Priority (feel, not every knob)

1. Fun to play.
2. **Always sounds good** on the default path (in key, voice-led HOME).
3. **Not stuck boring.** Jazzy / borrowed / “wrong” when the player *tries* (spice, type override, extra tensions). Never as the penalty for a missed slice or for holding the stick.

Center + the chord the mode asked for are the safe road. Interesting is a lean or a held extra.

---

## Buttons — Type (locked for this proto)

Same 8 Gaterons. In Type mode they are **not** I–vii.

| Bottom | Dim | Min | Maj | Sus |
| Middle | 6 | m7 | M7 | — |
| Top | 9 | Key | Shift | panic |

Incoming MIDI note is the **root**. Type buttons combine with extras. **Maj means maj** even if the key wanted minor. That is a legal way to *try* to go out. Stick still does not change type.

Extra proto keys are menu/options: **Key** (hold + note sets the key), **Shift** (reserved), RECORD = panic. Product hardware adds a mode toggle among the tactiles; no pots.

Later experiment, not this firmware: **smart Type** — the same type cluster, but qualities follow the note / key so you can mash Maj and still land in-key. Degree mode is the other way to stay in key (degrees on the buttons). Do not confuse them.

---

## Buttons — Degree (product mode, not this proto)

Same 8 Gaterons, different map. Buttons are scale degrees of the current key.

In C major, roughly: I C, ii D min, iii E min, IV F, V G, vi A min, vii B dim. HOME quality is diatonic. The eighth button is still open (high I vs spare).

No incoming note required to make a chord. A keyboard can still layer thru or set the key (Key + note).

Held extras (6 / m7 / M7 / 9) from Type mode do not apply as pad labels in Degree mode unless we later reuse the same physical keys with a Shift layer. Do not invent that until Degree HOME is musical with triad-only degrees.

Stick contract is **identical** to Type: color, not quality. ii + up is more open / in-key color on Dm, not Dmaj9.

---

## Hands

Box left of a keyboard. Left hand is M1.

1. **Buttons only** — complete at HOME. Stick optional. Type needs a root from MIDI; Degree does not.
2. **Buttons + thumb stick** — 2×4 cluster, stick on the inner edge where the thumb lives. Lean = color, release = HOME.
3. **Stick-only** — needs latched last degree / type.

Hold vs latch on the **buttons** is still open. This proto **assigns at note-on**: the type held when a key goes down sticks to that key until it comes up. Releasing the pad does not drop the chord. New keys with no pad are thru.

---

## Stick — contract

Hardware: analog, snap to 8 seats + center.

**Locked:** spring-back. Center = HOME. Lean is temporary. Release = HOME. Not sticky.

**Locked:** seats are not chord types. No static 8-quality table, **including in Degree mode**. That was the trash part.

The stick should be:

1. **Dynamic** — notes depend on current chord, key, and last HOME (voice leading). Same seat, different MIDI next bar.
2. **Predictable** — same gesture, same *kind* of thing.
3. **In key by default** — extra notes from the scale. ♯11 / dim-for-fun = spice, not a diagonal. Maj on a ii is Type-mode Maj (or spice), not the stick.
4. **Fun / not boring** — up should still do something you can hear (open voicing and/or in-key 9/11/13).

### Predictability (voicing stick)

- **Up** — more color / more open / more (in-key) tension. Never “become major.”
- **Down** — tighter / closer. Never “become minor.”
- **Left / right** — bass / inversion.
- **Center / release** — HOME.
- **Corners** — both axes, not a third identity.

If a tester cannot say what up does in one sentence, too clever.

### What the stick is not

- Not 8 chord qualities. Archive failure even with I–vii buttons. Degree mode does not bring this back.
- Not a progression picker.
- Not a key picker.
- Not sticky.

No lab “quality stick” as a user-facing mode. Compile-flag A/B against the old tables is allowed in private. Do not ship it.

---

## Stick — candidate map (try this)

**Role + context.** Seats are roles. Notes are picked smart:

Given: current chord (from Type buttons or Degree + key) + last HOME + seat  
Pick pitches that (1) keep that chord’s identity, (2) match the role, (3) voice-lead, (4) stay in key unless spice is open.

Example, C major, Type mode, MIDI D + Min held → Dm.

- HOME: D–F–A near the last chord.
- RIGHT: F in the bass, rest voice-led.
- UP: add in-key 9 (E) and/or open the spacing.
- Release: back to HOME, still Dm.

Same lean with MIDI G + Maj does that job to G, not “the same MIDI notes.”

Degree mode, C major, button ii: same Dm example, no MIDI D required.

Start proto here. If it feels dead, enrich the up-ladder (still in key) before bringing back quality slices.

---

## Tension ladder (up / down)

In Type mode the cluster already has 6 / m7 / M7 / 9, so up is (1) spacing and/or (2) unnamed in-key extras the buttons did not already hold. Held 9 always wins.

Degree mode has no extra pads at first, so the up-ladder *is* how you add in-key color.

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
- The chord the mode asked for, voice-led. Buttons-only at HOME must already sound like a product.
- Voice-lead from the previous HOME. Not a fresh closed pile every time.
- Top note biased into a useful band (around G3–G5). Firmware used to clamp everything toward middle C; do not do that. Follow the played octave (Type) or a sensible default register (Degree).

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

- Type mode: buttons ignore key for quality (honest Maj). Stick extras and spice still read key.
- Degree mode: key **is** the map. Wrong key makes the wrong degrees, which is correct.
- Override: hold **Key** + incoming MIDI note.
- Display: `Dm7` and `C major`, plus Type/Degree.

Wrong key in Type mode should only make stick extras weird, not change Min into Maj.

---

## Spice (the “I tried” door)

Stick click, Shift+stick, or a labeled edge. Borrowed color, secondary dominant, modal mixture. Player opened it. Diagonals are not spice. Half-the-stick-out-of-key is not spice.

v0 can ship spice = off and still not be boring if the in-key up-ladder is audible.

---

## MIDI (this proto)

Lab box: RP2040-Zero USB-C as MIDI **device** `OpenChord M1`. Launchkey Mini MK4 keys = roots. Pads = Type buttons (ch 10 CC 36–43). Two knobs = stick (CC 47/48), ignored until they pass center once.

Each piano key is assigned at **note-on**:

- Type pad held → that key is a chord (root not echoed). Stays until **that** key is released, even if the pad is let go.
- No type pad → **thru**.
- A new key does not revoice an already-held chord. Two type pads can hold two chords at once.

PC is USB host. Launchkey → DAW/router → OpenChord in. OpenChord out → instrument track. Do not also send Launchkey notes straight to the instrument.

Outgoing notes are ref-counted so a thru E on top of C major does not kill the chord’s E. Channel split later (plugin setting).

The plugin, when it exists, uses the same assignment rules. MIDI Learn remaps CC/notes; it does not change the engine. If a hardware M1 is present, the plugin does not render — see `goals.md`.

---

## v0 playtest

1. USB MIDI in = root. Hold Dim/Min/Maj/Sus (+ extras) and play a key. Chord stays until that key is up. (Type mode.)
2. Stick roles, spring-back, **in-key extras only**. Type never comes from the stick.
3. Key override (Key + note). OLED: chord + key + mode. (Product; not on the Zero proto yet.)
4. Thru when no type is held at note-on.
5. Degree mode after Type HOME is worth shipping. Smart Type later if Type is too easy to leave the key.

Do not add quality slices on the stick. That is the OG.

---

## Open questions

Locked: spring-back HOME. Stick ≠ static quality map in **both** modes. Stick extras in key. Type + Degree as product modes (Degree not this proto). Out of key is Type quality you held, or spice later. Plugin and firmware share this file.

Decided for this proto: Type buttons + MIDI root.

Still open:

1. Smart Type (types follow the note/key) — later.
2. Degree eighth button.
3. HOME = triad or always add a 7th?
4. Up: spacing, in-key extras, or both?
5. Keep role while leaning across a new type (probably yes)?
6. Hold vs latch on the physical buttons? Note-on assignment is what the RP2040 proto does.
7. Layout: thumb-reach stick?
8. Degree + Shift layer for 6/m7/M7/9, or leave extras to the stick?

When one is decided, update this file.
