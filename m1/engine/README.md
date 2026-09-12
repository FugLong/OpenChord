# M1 chord engine

Portable C++. No Daisy. No TinyUSB. No RP2040 headers.

`m1/proto-rp2040` (and later `m1/firmware`) only call this.

- Incoming MIDI note = root
- Buttons = Orchid type + extras (`Type`, `Ext` bits)
- Stick seat = voicing / in-key color, not quality
- `Render()` voice-leads toward the previous voicing

Spec: [docs/chord-engine.md](../docs/chord-engine.md).
