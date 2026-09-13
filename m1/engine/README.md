# M1 chord engine

Portable C++. No Daisy. No TinyUSB. No RP2040 headers. No JUCE.

`m1/proto-rp2040`, later `m1/firmware`, and `m1/plugin` only call this. If the hardware box is connected, the plugin must not call `Render()` on the same notes.

- `chord_engine` — `Render()`, `SeatFromStick()`, `DegreeToChord()`, triad Type / Ext / Seat
- `session` — Pro + Smart play modes, voice table, refcounted MIDI out, stick pickup, Key / panic (shared by proto + plugin)

- **Pro** mode: incoming MIDI note = root; pads = Dim / Min / Maj / Sus + extras
- **Smart** mode: pads = I–vii + high I; diatonic triad at HOME; hold while pressed
- Stick seat = voicing / in-key color, not chord type, in both modes
- `Render()` voice-leads toward the previous voicing

Spec: [docs/chord-engine.md](../docs/chord-engine.md). Product hosts: [docs/goals.md](../docs/goals.md).
