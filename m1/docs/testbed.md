# M1 testbed

How we prove the chord engine before the RP2040 board exists.

**Do not edit [`archive/s1-daisy/`](../../archive/s1-daisy/).** That tree is the restore image for the original prototype. Never commit changes there.

**Pins: match the firmware that runs, not the archive docs.** `docs/hardware/pinout.md` and `src/core/io/pin_config.h` are stale. The box matches `Init()` in `digital_manager.cpp`, `analog_manager.cpp`, `display_manager.cpp`, and `midi_handler.cpp`. Copy of that map: [`../proto-daisy/pin_map.h`](../proto-daisy/pin_map.h). Do not copy `pin_config.h` into the harness.

## What we are not doing

- Not borrowing the Xbox RP2040 adapters unless the Seed box is dead. They have no stick, no keys, no screen.
- Not waiting on mail to start. The engine does not need hardware.
- Not ESP32-C3. USB MIDI on C3 is a trap.
- Not putting this in `s1/`. S1 is the future studio product.

## Order

1. **`m1/engine`** — portable C. Host tests on a laptop. MIDI in/out can be fake. The logic is the product.
2. **`m1/proto-daisy`** — thin Seed firmware that *only* talks to the old box (keys, stick, OLED, USB MIDI) and calls `m1/engine`. No synth, no tracks, no old menu OS.
3. Play until C–Am–F–G never sounds stupid and ideas start showing up.
4. **`m1/firmware`** — same engine on RP2040 when the custom PCB is real.
5. Reflash the OG box from `archive/s1-daisy` when we are done with the harness.

If the engine is wrong on a laptop, the enclosure will not save it. If it is right on a laptop and wrong on the Seed, the harness is wrong, not the music.

## Restore the OG prototype

From repo root, with Daisy toolchain and submodules:

```bash
cd archive/s1-daisy
git submodule update --init --recursive
make
# flash with the usual Daisy / dfu flow from that tree
```

Do this once **before** the first proto-daisy flash if you want a known-good `.bin` on disk. Keep that binary out of `archive/` (put it in `m1/proto-daisy/restore/` locally if you want; it does not need to be committed).

## Seed box → M1 controls

Matrix is **3 rows × 4 cols** (11 keys). Row 0 = bottom, row 1 = middle (col 3 unused), row 2 = top. From `digital_manager.cpp` + `button_input_handler.cpp`.

| Where | Matrix | M1 |
|-------|--------|-----|
| Bottom 4 | row 0, col 0–3 | Dim, Min, Maj, Sus |
| Middle 3 | row 1, col 0–2 | 6, m7, M7 |
| Top INPUT | row 2, col 0 | **9** |
| Top INSTRUMENT | row 2, col 1 | **Key** |
| Top FX | row 2, col 2 | **Shift** |
| Top RECORD | row 2, col 3 | unused / panic |
| Stick X/Y | A2 / A3 | 8 voicing seats |
| Stick click | D0 pin 1 | spice, later |
| OLED | SPI as in pin_map.h | `Cmaj7` / `C major` |
| USB MIDI | EXTERNAL pins 36–37 | roots in, chords out |
| Encoder, audio, battery | — | ignore |

Do not port archive chord-mapping presets onto this map.

## Pins the harness must use

From the manager `.cpp` files, not `pin_config.h`:

| Function | Daisy | Header pin |
|----------|-------|------------|
| Joy X / Y | A2 / A3 | 24 / 25 |
| Joy click | D0 | 1 |
| Key rows bottom→top | D20, D21, D22 | 27, 28, 29 |
| Key cols 0–3 | D23, D24, D25, D26 | 30–33 |
| OLED CS / SCK / MOSI | D7 / D8 / D10 | 8 / 9 / 11 |
| OLED DC / RST | D13 / D14 | 14 / 15 |
| TRS MIDI RX / TX | PB8 / PB9 (D11 / D12) | 12 / 13 |
| Encoder A / B | D27 / D28 | 34 / 35 |

Stale `pin_config.h` still says joy click D14, display DC on D0, MIDI as D12/D13-the-names, and a 4-row matrix. That is not the box.

## Product hardware (not this testbed)

RP2040 on our PCB, USB-C on our edge, Gateron LP × 8, Alps C219778, two tactiles, LCSC OLED. KiCad bits for the stick are already in `m1/hardware/lib/` (LCSC C219778).
