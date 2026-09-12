# Chord engine (draft — write this next)

Portable logic for OpenChord M1. Implementation: `m1/engine`. Product rules: `goals.md`. Hands-on: `testbed.md`.

This file is the music spec. It is not done yet. Do not port the archive joystick quality table.

## North star

The most helpful chord/MIDI device we have used.

- Almost impossible to sound bad.
- Still surprising: which *legal* voicing, not random dim.
- Buttons are honest (maj means maj).
- Stick only colors (inversion / tension seats).
- Remembers the last chord and voice-leads.
- Stays in key unless the player opens a door (borrow / spice).
- Later: actually **helps with progressions and ideas**, not only the chord you are already holding.

v1 ships when voicing + voice leading + key feel magic. “Where should I go next?” can start as a quiet hint. Do not block v1 on a full songwriter AI.

## Layers (to spec next)

1. **Identity** — type + extension from the eight keys. Combinable. Hardcoded.
2. **Color** — 8 stick seats + home. Inversion / tension / both. All in-key unless spice.
3. **Voice leading** — common tones stay; others take the short path; no closed root-position stack every time.
4. **Key** — infer from what is played; override with Key + a note.
5. **Progression** — optional next-chord gravity (diatonic functions, cadences, borrowed color on purpose). This is the long game.

## Do not

- Eight pie slices of unique qualities (ii in C becoming Dmaj9).
- Diagonals as dim/aug punishment.
- Archive preset shuffle.
- A laptop plugin as a v1 requirement.

## Next

Sit down and fill this file: note sets, inversion table, tension ladder, voice-leading rules, MIDI channel plan, what “suggest next” even means. Then write `m1/engine` against it.
