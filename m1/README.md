# OpenChord M1

MIDI-only chord brain. Keyboard or DAW in, smarter chords out.

This is the first product in the OpenChord family. It does not make sound. The portable synth/jambox is [S1](../s1/), later.

**Read first:** [docs/goals.md](docs/goals.md). Then [docs/testbed.md](docs/testbed.md). Chord model: [docs/chord-engine.md](docs/chord-engine.md).

## Status

Design + engine about to start. No RP2040 board yet. Feel-testing will use the old Seed prototype **without editing the archive**.

## Tree

```
m1/
  docs/           goals, testbed, chord-engine
  engine/         portable chord logic (the product)
  proto-daisy/    Seed harness for the OG box (restore from archive)
  firmware/       RP2040 (later)
  hardware/       PCB, enclosure, KiCad libs (C219778 stick already here)
```

`s1/` stays empty. `archive/s1-daisy/` stays frozen.
