# OpenChord

A family of open hardware instruments for harmony and music-making. MIT licensed. **OpenChord M Core** (plugin) is free; the **OpenChord M1** box is what we sell.

**M-series** is MIDI. **S-series** is studio. Numbers are generations.

| Product | What it is | Status |
|---------|------------|--------|
| **[OpenChord M1](m1/)** | MIDI-only chord brain. RP2040 hardware under $100, plus free **OpenChord M Core** (AU/VST3, same engine). | Active — engine / proto / plugin v0 |
| **[OpenChord S1](s1/)** | Studio / portable jambox: synth, tracks, the original dream device. | Later |
| M2 / S2 | Next generations of each line. | Not started |

```
OPENCHORD          OPENCHORD
    m1                 s1
```

## Repo layout

```
m1/                     M1 engine, firmware, plugin, hardware, docs (start here)
s1/                     Reserved for the jambox when it is real
archive/s1-daisy/       Frozen Daisy Seed prototype (do not treat as S1)
LICENSE                 MIT
```

The Daisy Seed project that used to live at the repo root is intact under [`archive/s1-daisy/`](archive/s1-daisy/). It is a prototype archive, not the S1 codebase.

## Start with M1

Read [`m1/docs/goals.md`](m1/docs/goals.md) for the product plan (hardware + free plugin). Hands-on testing is [`m1/proto-rp2040`](m1/proto-rp2040/) (RP2040-Zero USB MIDI, Type mode). Plugin build: [`m1/plugin`](m1/plugin/) (**OpenChord M Core**). The archived Seed box is parked. Do not edit the archive.

## License

MIT — see [LICENSE](LICENSE).
