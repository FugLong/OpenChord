# OpenChord M1

MIDI-only chord brain. Keyboard or DAW in, smarter chords out. Hardware you buy; plugin you get for free. Same engine.

This is the first product in the OpenChord family. It does not make sound. The portable synth/jambox is [S1](../s1/), later.

**Read first:** [docs/goals.md](docs/goals.md). Then [docs/testbed.md](docs/testbed.md). Chord model: [docs/chord-engine.md](docs/chord-engine.md).

## Status

Engine is in `m1/engine`. Feel-testing is **`m1/proto-rp2040`** (RP2040-Zero, PlatformIO, USB MIDI, Type mode). Plugin and product firmware are not started. The Seed proto is parked. Do not edit the archive.

## Tree

```
m1/
  docs/           goals, testbed, chord-engine
  engine/         portable chord logic (the product)
  proto-rp2040/   RP2040-Zero lab harness (USB-C MIDI, Launchkey pads/knobs)
  proto-daisy/    Seed harness (parked; restore OG from archive)
  plugin/         AU MIDI FX + VST3 (later). Free. Links engine.
  firmware/       Product RP2040 (later)
  hardware/       PCB, enclosure, KiCad libs (C219778 stick already here)
```

`s1/` stays empty. `archive/s1-daisy/` stays frozen.
