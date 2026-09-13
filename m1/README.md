# OpenChord M1

MIDI-only chord brain. Keyboard or DAW in, smarter chords out. **Hardware you buy is M1**; the free DAW plugin is **OpenChord M Core**. Same engine.

This is the first product in the OpenChord family. It does not make sound. The portable synth/jambox is [S1](../s1/), later.

**Read first:** [docs/goals.md](docs/goals.md). Then [docs/testbed.md](docs/testbed.md). Chord model: [docs/chord-engine.md](docs/chord-engine.md). Plugin v0: [docs/plugin.md](docs/plugin.md).

## Naming

| Name | What |
|------|------|
| **OpenChord M1** | Hardware (USB MIDI device). Repo folder `m1/`. |
| **OpenChord M Core** | Free AU MIDI FX + VST3 in `m1/plugin/`. |
| **Pro** | Play mode: keyboard roots + chord-type pads |
| **Smart** | Play mode: pads = I–vii + high I, hold to play |

## Status

Engine is in `m1/engine` (including shared `session`). Feel-testing is **`m1/proto-rp2040`** (Pro). Plugin v0 lives in **`m1/plugin`** as **OpenChord M Core** (Pro + Smart). Product firmware is not started. The Seed proto is parked. Do not edit the archive.

## Tree

```
m1/
  docs/           goals, testbed, chord-engine
  engine/         portable chord logic (the product)
  proto-rp2040/   RP2040-Zero lab harness (USB-C MIDI, Launchkey pads/knobs)
  proto-daisy/    Seed harness (parked; restore OG from archive)
  plugin/         OpenChord M Core — free AU MIDI FX + VST3. Same engine + session as M1 hardware.
  firmware/       Product RP2040 (later)
  hardware/       PCB, enclosure, KiCad libs (C219778 stick already here)
```

`s1/` stays empty. `archive/s1-daisy/` stays frozen.
