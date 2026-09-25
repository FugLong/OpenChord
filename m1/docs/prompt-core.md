# Fresh session — M1 core, plugin as the test bed

Paste this whole file to a new agent. Repo: `/Users/fuglong/Documents/Repos/OpenChord`. Work under `m1/`. Do not edit `archive/` or `s1/`.

## Goal

Build the core of OpenChord M1 and prove it in **OpenChord M Core** (the plugin). There is no board yet. The plugin is the device emulator and the only test bed. A finger on the real box and a click in the plugin must be the same engine event.

Do not flash hardware. Do not write GPIO, I2C, or TinyUSB drivers in this session. Do structure `m1/firmware` so those drivers will call the same surface later.

## Read first

1. `m1/docs/interaction.md` — behavior. This wins over older docs.
2. `m1/docs/build.md` — order of work, and **Where we are**. Steps 1–7 are done in the plugin. Next is step 8, and only when asked.
3. `m1/firmware/board.h` — real rev A pins, for names and comments. Do not talk to them yet.
4. `m1/engine/chord_engine.h`, `session.h`, `session.cpp` — current engine.
5. `m1/plugin/` — JUCE host. OpenChord M Core. It already plays the surface.

`m1/docs/goals.md` is product intent and the hardware lock. Ignore as product spec: `m1/docs/chord-engine.md`, `m1/docs/plugin.md`, `m1/firmware/README.md`. They still say Pro, Smart, pads, and panic. Do not revive those words.

## Names (locked)

| Word | Means |
|------|--------|
| **keyswitch** | One of 8 Gateron switches. Not a pad. |
| **button** | One of 3 edge tactiles: Prev, Menu, Next. |
| **trackpad** | XY. |
| **strip** | Slider. |
| **Keys** | Mode. Incoming MIDI note is the root. Keyswitches are Dim, Min, Maj, Sus, 6, m7, M7, 9. None held = Thru (echo the note). |
| **Scale** | Mode. Keyswitches are I ii iii IV / V vi vii I+. Hold plays that degree. |
| **Drums** | Mode. Labels may change. Hits do not have to sound yet. |
| **Bind** | Plugin-only. Not a control on the box. |

No other companies’ product names in UI, comments, or new docs.

## What to build

One input surface in `m1/engine`. Plugin and future firmware both call it. No JUCE and no Pico headers in the engine.

| Control | Call | Plugin, Bind off |
|---------|------|------------------|
| Keyswitch 1–8 | down / up | Mouse down is held, mouse up is released. Latch, or Option/Control, sticks keyswitches only. |
| Prev, Menu, Next | down / up | Momentary. They never stick. |
| Trackpad X, Y, finger | −1..+1, touching | Drag, then let go. The value stays and the finger stays down. |
| Strip | 0–255, touching | Plays while the mouse is down. Mouse up lifts the finger. |

**Bind** is a plugin-only button. Click it, then click one control (a keyswitch, a button, trackpad X, trackpad Y, or the strip). The next MIDI note or CC assigns that control. Clicks in this mode do not play. Click Bind again to cancel. A successful assign leaves Bind. One message belongs to one control. Right-click while Bind is on clears that control. A reset restores a Launchkey preset: their eight pads → keyswitches 1–8, two knobs → trackpad X/Y. That preset is a convenience.

CC value ≥ 64 means held. A piano key, a pad, or a joystick axis are all legal. The binding does not change when the mode changes.

Prev / Next cycle **Keys → Scale → Drums → Keys** and release every note we sounded. Menu opens and closes and also releases our notes. Keyswitch labels follow the mode.

Current play rules are interaction.md. Keys with a type held turns the incoming note into that chord. No keyswitch passes the notes through and names the chord. Scale sounds the newest held degree. Drums has two banks. Do not restart steps 1–7 unless asked.

Firmware: a small core that feeds the same surface from a plain input struct (eight keyswitch bools, three buttons, trackpad, strip). No pins toggled. The plugin remains the thing you run.

## Done when

- Plugin builds (CMake, existing `m1/plugin` target).
- Mouse: hold Maj, play C, hear a major chord. Release Maj and the next note passes through.
- Mouse: Prev/Next changes mode and the eight labels change. A sounding note does not survive the change.
- Bind: assign an arbitrary note or CC to a keyswitch without that click playing. Play that message afterward and it toggles the keyswitch.
- Trackpad drag keeps its value after mouse-up. A second click lifts it.
- `m1/engine` still has no JUCE and no RP2040 headers.
- `archive/` untouched.

## Do not build this session

Color table, Harmony, Strum, Vary, drum note numbers, OLED drawing, USB device detection, TRS, photos of the case, arp. Steps 3 onward in `m1/docs/build.md`.
