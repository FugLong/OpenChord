# OpenChord

A family of open hardware instruments for harmony and music-making.

**M-series** is MIDI. **S-series** is studio. Numbers are generations.

| Product | What it is | Status |
|---------|------------|--------|
| **[OpenChord M1](m1/)** | MIDI-only chord brain. RP2040, custom PCB, USB-C on our edge. Target under $100. | Active — engine / proto testbed |
| **[OpenChord S1](s1/)** | Studio / portable jambox: synth, tracks, the original dream device. | Later |
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

Read [`m1/docs/goals.md`](m1/docs/goals.md) for the product plan. Hands-on testing uses the frozen Seed box as a harness; that work lives in `m1/`, not in the archive.

## License

MIT — see [LICENSE](LICENSE).
