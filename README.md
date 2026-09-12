# OpenChord

A family of open hardware instruments for harmony and music-making.

**M-series** is MIDI. **S-series** is sound. Numbers are generations.

| Product | What it is | Status |
|---------|------------|--------|
| **[OpenChord M1](m1/)** | MIDI-only chord brain. Sits between a keyboard (or DAW) and the rest of the rig. No audio. | Active — design |
| **[OpenChord S1](s1/)** | Portable jambox: synth, tracks, the original dream device. | Later |
| M2 / S2 | Next generations of each line. | Not started |

```
OPENCHORD          OPENCHORD
    m1                 s1
```

## Repo layout

```
m1/                     M1 firmware, hardware, and docs (start here)
s1/                     Reserved for the jambox when it is real
archive/s1-daisy/       Frozen Daisy Seed prototype (do not treat as S1)
LICENSE                 MIT
```

The Daisy Seed project that used to live at the repo root is intact under [`archive/s1-daisy/`](archive/s1-daisy/). It is a prototype archive, not the S1 codebase.

## Start with M1

Read [`m1/docs/goals.md`](m1/docs/goals.md) for the full product plan: what it is, what it is not, hardware, chord engine, and why this exists instead of finishing the Seed box first.

## License

MIT — see [LICENSE](LICENSE).
