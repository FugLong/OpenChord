# OpenChord M1 — goals and plan

Written September 2026 so we do not lose the decisions that led here. This file is the product intent and the hardware lock. Behavior lives in [interaction.md](interaction.md). What is built and what is next lives in [build.md](build.md). Firmware lives under `m1/`. Do not edit `archive/s1-daisy/`.

**Working name:** OpenChord M1 (`OPENCHORD / m1`)

**Family:** OpenChord. **M** = MIDI tools. **S** = studio. Numbers are generations. The jambox, when it exists, is **S1**. Later we can do M2 / S2 without renaming anything.

---

## Why this exists

The original OpenChord was a portable Daisy Seed jambox: synth, tracks, looper, sampler, OLED, battery, 11 keys, joystick. A prototype was built. It taught us a lot. It is not the first thing to ship.

Reasons we paused the full dream:

- Daisy Seed audio on the proto never got clean (wiring, PowerBoost, grounds — the module fight was not worth another year).
- The kitchen-sink spec (four live engines, USB host + device, granular + wavetable + analog, audio looper) does not match a first sellable board.
- The author is a **DAW person**. MIDI into a computer is how the music actually gets made. A DAWless studio-in-a-brick was a fantasy of a device that would finally make DAWless feel good. At the end of the day the DAW won.
- The interesting idea was never the synth. It was **smart, dynamic chord mapping** that stays musical.

So: a smaller, MIDI-only product that can live on a custom PCB and actually be sold, plus a free plugin that runs the same engine. S1 stays in the family for later.

The Daisy tree is archived at [`../../archive/s1-daisy/`](../../archive/s1-daisy/). Do not treat it as S1 source.

---

## What M1 is

A **man-in-the-middle MIDI enhancer**. Feed it MIDI from a real keyboard, a controller, or the DAW. It sends MIDI back transformed: chords, voicings, extensions, voice leading.

Two hosts, **one engine** (`m1/engine`):

- **Hardware** — RP2040 box, USB-C MIDI device, later TRS. The thing you buy. USB name: `OpenChord M1`.
- **Plugin** — **OpenChord M Core**, free, MIT, AU MIDI FX + VST3. Same `oc::Render()`. MIDI Learn so any pads/stick/knobs can be the cluster.

- No audio (neither host).
- No USB host on the box. The PC is the USB host. M1 hardware is a USB MIDI **device**.
- Hardware: TRS MIDI in and TRS MIDI out (plus USB MIDI in/out).
- Bus-powered from USB-C. **No battery** on v1. If TRS-only gear needs it, plug USB-C into power with no data.
- Small enough to sit on a desk next to a keyboard and disappear into a session.

One-line pitch: *sits between your keys and the DAW and makes you sound like you know theory.*

Ambition: the most helpful chord/MIDI tool we have used. Not a quality roulette. It should voice well, stay in key, and later help with **where to go next** (progressions, ideas). Voicing that never sounds stupid is v1. Progression-smart is the north star; do not fake it with random borrowed chords. Write the model in [chord-engine.md](chord-engine.md).

---

## What M1 is not

- Not a synth, sampler, looper, or drum machine.
- Not the 11-key standalone piano from the Seed proto.
- Not a Daisy **product** and not a Pi/CM5 project. The old Seed box is a temporary M1 testbed only. See [testbed.md](testbed.md).
- Not USB MIDI host (no “plug a controller into M1 with no computer” as a v1 requirement).
- Not the old OpenChord hierarchical menu on the box. Extra settings live in the **plugin**, not extra knobs on the enclosure.
- Not a battery synth with speakers. We are a studio **MIDI** tool: great at the MIDI path, no sound of our own.
- Not a clone of other chord pads or slab synths. **Scale** is I–vii with our own color map and voice leading — designed here, named here. Do not put other companies’ product names in the UI, marketing, or source comments.

If a feature needs audio or a big on-device screen, it belongs on **S1**, not a “quick add” to M1. If it is a MIDI preference (channel split, voicing tightness, maps), it belongs in the plugin.

---

## Who it is for

First user is us: MIDI or live instruments into a DAW. Second user is anyone who wants left-hand chord control without buying a synth they will not use. The plugin is how people try it with a Launchkey (or anything). The box is for people who want that control in their hands.

In **Keys** the keyboard (or the DAW piano roll) chooses the root, or passes straight through when no keyswitch is held. In **Scale** the eight keyswitches are the degrees and a keyboard is optional. In **Drums** they are hits. Either way M1 is the left hand and the brain.

---

## Interaction

Three modes, one surface. The rules are locked in [interaction.md](interaction.md). Do not revive Pro, Smart, Follow, Play, or Jazz.

| Mode | Who it is for | What the eight keyswitches are |
|------|----------------|--------------------------------|
| **Keys** | A keyboard into the box or the plugin | Chord type: Dim, Min, Maj, Sus, plus 6, m7, M7, 9. No keyswitch = the notes pass through, and the screen names the chord they form |
| **Scale** | Both hands on the box. No keyboard required | Degrees I–vii and I one octave up. The newest one held is the chord. Older ones stay down and come back when you let go |
| **Drums** | Hits, not chords | Two kits. Trackpad X picks the bank |

The trackpad is not a quality roulette. In Keys it picks bass and spread. In Scale it picks one of eight in-key chords around a safe center triad. Harmony (Tensions, Borrowed, Free) is specified and not built yet. In Drums, X is the bank and Y is velocity.

Strum is a menu setting, not a mode. **Optional** (default) lets the chord sound and the strip add plucks. **Only** waits for the strip.

The screen is two lines, the same on the plugin and the OLED: what will sound, then key, mode, and status. Menu rows are Key, Octave, Vary, Harmony, Strum.

**Hardware plan (rev A schematic):** capacitive **trackpad** on **IQS572BLQNR** (I²C `0x74`). **AT42QT2120-XUR** does the touch **strip only** (KEY0–2 slider). **No Key / Shift copper** on the QT. **3× EVQPUC02K** are Prev, Menu, and Next. Shared bus is **I2C1** on GPIO10/11 (OLED `0x3C` + QT `0x1C` + IQS `0x74`; IQS RDY GPIO8, NRST GPIO9). Alps **RKJXV1220001** / LCSC **C219778** stays in the KiCad lib as the fallback if the trackpad feel fails. Pin map: [`../hardware/support.md`](../hardware/support.md).

No encoder on v1. **No pots / dials on the box.** They add size and cost, and this product assumes a DAW.

The three edge buttons are **Prev**, **Menu**, and **Next** (SW11 GPIO3, SW9 GPIO4, SW10 GPIO5). Outside the menu they change mode. Inside it they move between settings. There is no panic button. Holding Menu inside the menu flushes stuck notes.

Key is a menu row. The next incoming note while that row is open sets the key and is not performed.

The box plays with no plugin. The plugin is how you bind controllers and, later, how you watch the box. When `OpenChord M1` is on USB the plugin stops voicing and mirrors one surface.

---

## Hardware (v1)

| Piece | Plan |
|-------|------|
| MCU | **RP2040** on our PCB (TinyUSB MIDI device, UART TRS). Not ESP32. Not a Pico glued to a carrier. |
| USB | USB-C on the **edge of our board**, device only, also power |
| MIDI | TRS in, TRS out; USB MIDI in/out |
| Front | **8× Gateron LP** (hotswap) — **direct GPIO** each (internal pull-ups; no matrix / no diodes). Cap: **IQS572BLQNR** trackpad + **AT42QT2120-XUR** strip only (KEY0–2). Share **I2C1** (GPIO10/11) with OLED (`0x3C` / `0x1C` / `0x74`; IQS RDY GPIO8). **3× EVQPUC02K**: Prev, Menu, Next. PCB copper electrodes; printed cover with openings. Alps stick = KiCad fallback only (not on rev A). **No pots / SoftPot.** |
| Display | Cheap 0.91" I2C SSD1306 (Ali module on PCB). |
| Audio | None |
| Battery | None |
| Host USB | None |
| Cost | Proto (buy 5): **~$34**/unit, **~$171** for five, landed. Street under **$100**. Breakdown: [`../hardware/bom.md`](../hardware/bom.md) (2026-09-19 carts). |

One custom PCB. **Rev A proto:** 4-layer, **HASL**, soldermask over touch copper (not ENIG). Hot air for RP2040 + IQS572 + QT2120. Through-hole for TRS (bushings **off** the board edge) and Gaterons via sockets. Reproducible. Prove touch on **coupons / TPS43** before a later product fab. Details: [`../hardware/touch.md`](../hardware/touch.md), [`../hardware/bom.md`](../hardware/bom.md).

**Prove the engine before that board.** Portable logic first. Hands-on feel on the **RP2040-Zero proto** (`m1/proto-rp2040`) with a Launchkey. Seed box is parked on archived OG firmware. See [testbed.md](testbed.md).

---

## Plugin (free)

Same product, second host. Code: `m1/plugin/`. Engine stays `m1/engine`. v0 implementation brief: [plugin.md](plugin.md).

**Sell:** the box. **Give away:** the plugin, MIT, like the rest of the repo.

### Formats

Ship both from one project (JUCE is the practical stack):

| Format | Who |
|--------|-----|
| **AU MIDI FX** | Logic, GarageBand — real MIDI insert |
| **VST3** | Cubase, Reaper, Bitwig, Studio One as MIDI FX where the host allows it. Ableton / FL: instrument-style wrapper that **outputs MIDI** onto another track (Scaler-shaped routing). |
| Standalone | Optional; IAC / loopMIDI last resort |

Do not ship AU-only. Most DAWs cannot host an AU MIDI effect.

### Who runs the engine

| | No hardware | Hardware on USB as `OpenChord M1` |
|--|--|--|
| **Engine** | Plugin | Box |
| **Plugin** | MIDI FX + Learn + extra settings | Bridge / editor / live view. **Does not voice.** |

Never run two engines on the same notes. Doubled chords and fighting voice-leading.

When the box is connected, extra settings can push to the device over MIDI (SysEx is the boring, correct path). Firmware update can live here later.

### Mapping and UI

MIDI Learn on the eight keyswitches, Prev, Menu, Next, trackpad X/Y, and the strip. One message, one control. A Launchkey preset (pads CC 36–43, knobs CC 47/48) is a convenience, not the product model.

The plugin draws the same two screen lines as the OLED, plus the eight keyswitches, the three buttons, the trackpad, and the strip. Ugly is fine.

Pretty overlay of the real enclosure is **later**, when the PCB exists. Do not skin the plugin as someone else’s device.

### Settings that are plugin knobs, not hardware dials

The menu on the box and in the plugin is the same: Key, Octave, Vary, Harmony, Strum. Extra ideas (channel split, other scales, shortcuts) wait. They are not a second set of knobs.

The box keeps defaults that already sound good. The plugin is the mixer.

---

## Chord engine — what we are fixing

The archived Seed mapper (`archive/s1-daisy` chord engine + joystick presets) had the right *idea* and a bad *feel*:

- Stick **replaced** quality instead of coloring the diatonic chord.
- Eight unique qualities, nastier ones on diagonals, analog stick cannot hit eight slices.
- No memory of the previous chord, no voice leading. Closed root-position stacks every time.
- Presets were the same eight qualities shuffled.

M1’s engine should make it **almost impossible to sound bad** and **still surprising**, then get good at **progressions**. That is the product. Do not port the old preset tables as the v1 model. **Scale** is how we put I–vii on the keyswitches: eight chords around a center triad, loosened only by Harmony. The live rules are in [interaction.md](interaction.md).

Useful scraps in the archive: scale/mode tables, interval lists, enclosure photos, “chord as an input plugin” as a concept. Not the 8-way quality map.

---

## Positioning

| Kind of product | Why we are not that |
|-----------------|---------------------|
| Battery synth with speakers | We are a studio **MIDI** tool. No audio of our own. |
| Chord-slab synth | We output MIDI. Scale is I–vii with a color map designed here. |
| Huge multi-button chord cockpit | We stay small and under $100. |
| Paid software-only chord assistants | We ship a **free** plugin and a **hardware** left hand. |
| Old OpenChord jambox proto | That is S1. |

Sell path: a board we can spin and price **under $100**, not a synth price. Plugin is free on purpose. MCU and radio were never going to be the margin. Enclosure, assembly, and not certifying Wi‑Fi are. **Assembly time matters:** one main PCB, **no paid PCBA** (you hand-build), no wiring harnesses, OLED soldered on-board — finishing a unit is not hours of cables or factory fees.

Name and domain: `openchord.com` is someone else’s music-apps site. Product name is still OpenChord M1; do not assume the domain. Do not put other companies’ product names on the hardware, plugin, store page, UI, or source comments.

---

## Repo and process

- Family lives in this repo (`OpenChord`). No product branches.
- M1 work stays under `m1/`.
- `s1/` stays empty until the jambox is a real project. **Do not dump M1 proto firmware there.**
- `archive/s1-daisy/` is frozen. Read it. Do not edit it. Restore the OG box by rebuilding that tree.
- Seed proto pins follow the archive **driver Init()** functions, not `pin_config.h` or `docs/hardware/pinout.md`. See [testbed.md](testbed.md) and `m1/proto-daisy/pin_map.h`.

```
m1/engine/          portable chord logic (no Daisy, no TinyUSB, no JUCE). This is the product.
m1/proto-rp2040/    RP2040-Zero lab harness. USB-C MIDI device. Not the product firmware.
m1/proto-daisy/     parked Seed harness. Restore OG from archive; do not edit archive.
m1/plugin/          OpenChord M Core — AU MIDI FX + VST3. Links m1/engine. Free.
m1/firmware/        RP2040 product firmware (later)
m1/hardware/        PCB / enclosure / KiCad libs
m1/docs/            this file, testbed, chord-engine
```

Build order, and what is already done, is [build.md](build.md). Short version: the plugin plays Keys, Scale, and Drums. Next is firmware on the rev A board, then the plugin mirroring that box. Do not build the photoreal UI before the box exists.

---

## Open decisions (on purpose)

These are not forgotten. They are not locked.

- Cap geometry (ICs locked — **IQS572BLQNR** + **AT42QT2120-XUR**): trackpad size / diamond pitch / deadzone; strip electrode geometry (slider = 3 interleaved → 0–255); **rev A = HASL + mask over copper**; ENIG gold electrodes = later SKU if feel needs it.
- Trackpad vs Alps stick: trackpad is on the board; C219778 remains the mechanical fallback in the lib (not populated on rev A).
- Harmony past **In key** (Tensions, Borrowed, Free). The tables are written. The engine still plays the In key map for every step.
- Other scales than major. Menu shortcuts. Drum rebinding. Arp. A photo skin of the enclosure.
- How much “next chord” suggestion is v1 vs later. The north star is still progressions, not a random borrowed chord.
- Trademark / `openchord.com` is someone else’s music-apps site.

Locked (do not reopen without updating this file and [interaction.md](interaction.md)): MCU RP2040; custom PCB with USB-C on the edge; no radio; no battery; no USB host on the SKU; **8 Gateron LP** keyswitches (**direct GPIO**, not a matrix); modes **Keys**, **Scale**, **Drums**; Scale 8th keyswitch = I one octave up; Scale center = the correct triad; newest held Scale keyswitch wins; Keys with no keyswitch names the incoming chord; Strum is **Optional** or **Only**; menu is Key, Octave, Vary, Harmony, Strum; edge buttons are Prev, Menu, Next; cap ICs = **IQS572BLQNR** (trackpad) + **AT42QT2120-XUR** (**strip only**); shared **I2C1** GPIO10/11; **3× EVQPUC02K**; USB-C = **TYPE-C-31-M-12** (C165948) **J1**; TRS = **SJ1-3523N** ×2 (**J2 OUT / J3 IN**, bushings off-board); flash **W25Q32JVSSIQ** (C179173); rev A proto **4-layer HASL** ~119 × 63 mm, SMT on **B.Cu**, hand-build (no paid PCBA); **no pots / no encoder / no SoftPot**; no QT Key/Shift copper; cheap OLED (GND/VCC/SCL/SDA); ~$99 hardware; plugin free (AU MIDI FX + VST3, same engine); hardware works with no plugin; if the box is connected the plugin does not voice; no competitor product names in UI/docs/code comments; engine portable; archive frozen. Pin map and passives: [`../hardware/support.md`](../hardware/support.md). Fab: [`../hardware/fab/jlcpcb/`](../hardware/fab/jlcpcb/).

When one of these is decided, update this file. Do not start a second source of truth.
