# M1 RP2040-Zero proto

USB-C on the Zero is a MIDI **device** named `OpenChord M1`. This harness is **Type** mode only. Launchkey Mini MK4 **keys** are roots. **Pads** are Type buttons (triad + extras). Two **knobs** are the stick. Degree mode is later (see `m1/docs`).

USB-A / qwerty host is parked. Product M1 still has no USB host.

## NeoPixel (GP16)

| Color | Meaning |
|-------|---------|
| Magenta | boot |
| Red blink | alive, USB-C not enumerated |
| Dim cyan | MIDI up |
| Green flash | MIDI in |
| White | type pad held, or a chord still ringing |

## Launchkey Mini MK4 37

Route: Launchkey → DAW **into OpenChord M1** (not also onto the instrument). OpenChord out → the instrument. Hold Maj pad, play C → C major.

### Keys

Notes on the piano keys = **root**. A type pad held at **note-on** assigns that key a chord. The chord stays until that key is released. No type at note-on = thru. Pads never make sound.

### Pads — channel 10 CC 36–43

Pads are **type only**. They never send or trigger notes.

Type is captured **when the piano key goes down**. That chord stays until you release **that key**, even if you let go of the pad. New keys while no pad is held come through raw. A pad held for a new key starts a separate chord and does not steal the first one.

| CC | Function |
|----|----------|
| 36 | Dim |
| 37 | Min |
| 38 | Maj |
| 39 | Sus |
| 40 | 6 |
| 41 | m7 |
| 42 | M7 |
| 43 | 9 |

36–39 are the triad. 40–43 only add color **with** a type (Maj + M7 = maj7). Extensions alone do not make a chord. Hold type + key, then tap an extra: the extra notes add in without retriggering the triad. The plugin’s Launchkey preset should match this map.

Value ≥ 64 = held. Channel 10 notes are ignored (not roots).

### Knobs — stick

| MIDI | Axis |
|------|------|
| Ch 1 CC 47 | stick X |
| Ch 1 CC 48 | stick Y |

64 is center (HOME voicing). Knobs that sit at 0 are ignored until you sweep them through center once, then they track. Away from center picks an 8-way seat (inversion / open). Chords follow the octave you play.

## Build / flash (PlatformIO)

```bash
export PATH="$HOME/.platformio/penv/bin:$PATH"
cd m1/proto-rp2040
pio run
```

UF2: `.pio/build/rp2040zero/firmware.uf2`

1. Hold **BOOT** on the Zero, plug USB-C (or BOOT + RESET).
2. `RPI-RP2` appears.
3. Copy the UF2 onto it.
