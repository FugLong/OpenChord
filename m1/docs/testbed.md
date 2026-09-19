# M1 testbed

How we prove the chord engine before the custom M1 PCB exists.

**Do not edit [`archive/s1-daisy/`](../../archive/s1-daisy/).** That tree is the restore image for the original prototype. Never commit changes there.

**Pins: match the firmware that runs, not the archive docs.** `docs/hardware/pinout.md` and `src/core/io/pin_config.h` are stale. The box matches `Init()` in `digital_manager.cpp`, `analog_manager.cpp`, `display_manager.cpp`, and `midi_handler.cpp`. Copy of that map: [`../proto-daisy/pin_map.h`](../proto-daisy/pin_map.h). Do not copy `pin_config.h` into the harness.

## What we are not doing

- Not the Seed enclosure USB for MIDI. That port is done. OG firmware is back on the box.
- Not ESP32-C3. USB MIDI on C3 is a trap.
- Not putting this in `s1/`. S1 is the future studio product.

## Order

1. **`m1/engine`** — portable C. Host tests on a laptop. MIDI in/out can be fake. The logic is the product.
2. **`m1/proto-rp2040`** — Waveshare RP2040-Zero, PlatformIO. USB-C = MIDI device. Launchkey Mini MK4 pads/knobs fake the Pro cluster and stick.
3. Play until C–Am–F–G never sounds stupid and ideas start showing up.
4. **`m1/plugin`** — **OpenChord M Core**, same engine in AU MIDI FX + VST3. Learn + extra settings. Skip voicing if a hardware M1 is present.
5. **`m1/firmware`** — same engine on the custom PCB. USB device only. No host on the SKU. No pots. **QT2120 strip only**; **3× EVQPUC02K** for Key / Shift / Mode (roles TBD). **I2C1** on GPIO10/11. See [`../hardware/support.md`](../hardware/support.md).
6. The Seed box stays on archived OG firmware unless we explicitly go back.

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

## USB MIDI (RP2040-Zero)

Native USB-C on the Zero is the MIDI **device**. Launchkey Mini MK4 pads (ch 10 CC 36–43) are types. Knobs CC 47/48 are the stick. Type is locked per key until that key is released.

Build/flash: [`../proto-rp2040/README.md`](../proto-rp2040/README.md).

Playtest routing:

1. Zero USB-C into the Mac. Device name: **OpenChord M1**.
2. Launchkey / DAW MIDI **into OpenChord** (do not also send those notes to the instrument).
3. OpenChord MIDI **out to the instrument track**.

No type at note-on = thru. Type held at note-on = that key becomes a chord until it is released. Later keys do not steal an already-held chord.

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

RP2040 on our PCB, USB-C on our edge, Gateron LP × 8, **IQS572BLQNR** trackpad + **AT42QT2120-XUR** strip only, **3× EVQPUC02K** system (roles TBD; Alps stick is a lib fallback, not on rev A), no pots / SoftPot, Ali OLED on **I2C1**. Free plugin is the settings surface. See [`../hardware/`](../hardware/).
