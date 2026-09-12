# M1 Seed proto harness

Throwaway firmware for the **original OpenChord box** (Daisy Seed, 11 keys, stick, OLED, USB MIDI). Implements the **new** M1 design on that hardware:

- Incoming USB MIDI note = **root**
- Eight chord keys = **Orchid** types (not I–vii)
- Stick = voicing / in-key color, spring-back HOME
- Chords go back out USB MIDI to the DAW

Pins: [`pin_map.h`](pin_map.h), copied from the archive **drivers**, not `pin_config.h`. **Must not modify `archive/s1-daisy/`.** Restore the OG synth from that archive when this harness is done.

See [docs/testbed.md](../docs/testbed.md) and [docs/chord-engine.md](../docs/chord-engine.md).

## Buttons

| Where | M1 |
|-------|-----|
| Bottom 4 | Dim, Min, Maj, Sus |
| Middle 3 | 6, m7, M7 |
| Top INPUT | 9 |
| Top INSTRUMENT | **Key** (hold + a MIDI note sets the key) |
| Top FX | Shift (unused in this build) |
| Top RECORD | panic (CC 123) |

Hold a type, play a note on the keyboard. No type = that note **thrus**. Type + note = chord **replaces** the root (the root is not echoed). Stick lean updates the sounding chord live.

## Build

Daisy toolchain (`arm-none-eabi-gcc`) and libDaisy submodule:

```bash
git submodule update --init archive/s1-daisy/lib/libDaisy
cd m1/proto-daisy
make
```

Binary: `m1/proto-daisy/build/OpenChordM1.bin`

This image is `BOOT_QSPI` so the box's existing USB-stick bootloader can load it.

## Flash (USB stick, same as OG)

The enclosure USB-C is **EXTERNAL** Seed pins 36–37 (`D29`/`D30`). The bootloader looks for a file named `OpenChord.bin`.

```bash
cd m1/proto-daisy
make
./flash_usb.sh "/Volumes/YOURSTICK"
```

Eject the stick, plug it into the **box** USB-C, power cycle. Do not use the Seed's tiny onboard USB for MIDI — that is not the port this firmware listens on.

## Play it

PC is USB host. After flashing, plug the **box USB-C** into the PC. The device should show up as USB MIDI (Daisy name).

Route:

1. Keyboard → DAW/router **into OpenChord** (not also onto the instrument track)
2. OpenChord MIDI out → the instrument / MIDI track

Hold Min, play C on the keyboard → Cm out. Lean the stick for inversion / in-key color. Hold Key + a note to set `C major` etc.

RECORD = all notes off. Restore OG firmware from `archive/s1-daisy` when you are done.
