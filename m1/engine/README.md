# M1 chord engine

Portable C++. No Daisy. No TinyUSB. No RP2040 headers. No JUCE.

`m1/proto-rp2040`, later `m1/firmware`, and later `m1/plugin` only call this. If the hardware box is connected, the plugin must not call `Render()` on the same notes.

- Type mode: incoming MIDI note = root; buttons = triad + extras (`Type`, `Ext` bits)
- Degree mode (later): buttons = I–vii; quality from the key at HOME
- Stick seat = voicing / in-key color, not quality, in both modes
- `Render()` voice-leads toward the previous voicing

Spec: [docs/chord-engine.md](../docs/chord-engine.md). Product hosts: [docs/goals.md](../docs/goals.md).
