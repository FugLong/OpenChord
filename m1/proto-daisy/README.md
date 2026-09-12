# M1 Seed proto harness

Throwaway firmware for the **original OpenChord box** (Daisy Seed, 11 keys, stick, OLED, USB MIDI).

- Calls `m1/engine`.
- Pins: [`pin_map.h`](pin_map.h), copied from the archive **drivers**, not `pin_config.h` or `docs/hardware/pinout.md` (those are outdated).
- **Must not modify `archive/s1-daisy/`.**
- Restore the OG synth/jambox firmware from that archive when this harness is done.

See [docs/testbed.md](../docs/testbed.md).
