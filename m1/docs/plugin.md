# OpenChord M Core — plugin v0 brief

Hand this file to a fresh agent and tell them to implement it. Product locks live in [goals.md](goals.md). Music model: [chord-engine.md](chord-engine.md). Engine API: [`m1/engine/chord_engine.h`](../engine/chord_engine.h). Feel that already works: [`m1/proto-rp2040/src/main.cpp`](../proto-rp2040/src/main.cpp).

Do not invent a second chord engine. Do not edit `archive/s1-daisy/`. Do not name modes Orchid or HiChord in the UI.

**Naming lock**

| Name | What |
|------|------|
| **OpenChord M1** | Hardware box (USB MIDI device you buy) |
| **OpenChord M Core** | Free AU/VST3 plugin (this brief) |

Do not call the plugin “M1” in the UI or bundle name.

---

## Why this exists

OpenChord M1 (hardware) and **OpenChord M Core** (plugin) share one MIDI chord brain. Incoming notes become roots. Eight Type buttons pick Dim / Min / Maj / Sus plus extras 6 / m7 / M7 / 9. A stick (two axes) colors voicing. `oc::Render()` in `m1/engine` does the music.

Two hosts, one engine:

- **Hardware** — RP2040 box (still a Launchkey + RP2040-Zero proto). What we sell. Device name: `OpenChord M1`.
- **Plugin** — **OpenChord M Core**, free, MIT. What DAW people use with whatever controller they have. This brief.

The plugin is not a synth. It does not make audio. It transforms MIDI.

The author uses **Logic**. Logic can host a real **AU MIDI FX**. Most other DAWs cannot. We also ship **VST3** so Reaper / Bitwig / Cubase / Ableton have something to load. Same C++.

Hardware still works with no plugin. This v0 does **not** talk to the box. When that exists, a connected `OpenChord M1` means **M Core** must stop voicing (see goals.md). Skip that detection for v0.

---

## v0 — must work

1. **AU MIDI effect** that Logic loads as a MIDI FX on an instrument (or on a MIDI FX slot).
2. **VST3** from the same code, for everyone else.
3. **Same chord feel as the RP2040 proto** (Type mode only).
4. **Ugly UI** that is usable: Type buttons, extras, stick, key, chord name, MIDI Learn.
5. **MIDI Learn:** click a control in the plugin, then press / move the matching thing on a MIDI controller. That message becomes the binding. No typing CC numbers.

If those five work in Logic with a Launchkey, v0 is done.

---

## v0 — do not build

- Degree mode (I–vii). Specced, not this plugin pass.
- Pretty picture of the hardware.
- Hardware USB detect / SysEx / firmware update.
- Channel split, smart Type, spice, progression suggestions.
- AAX, VST2, LV2, iOS AUv3.
- Qwerty computer-keyboard host.
- Anything in `archive/` or `s1/`.

Ugly JUCE `TextButton` / `Slider` / `Label` is correct. Do not spend time on look.

---

## Formats (one JUCE project)

CMake + JUCE 8. `FORMATS AU VST3 Standalone` is fine (Standalone helps debug without a DAW).

VST3 has **no** MIDI-FX type. Do not set `IS_MIDI_EFFECT TRUE` on the whole target or Ableton / many VST3 hosts will refuse it.

Use mixed flags:

```
IS_SYNTH TRUE
IS_MIDI_EFFECT FALSE
NEEDS_MIDI_INPUT TRUE
NEEDS_MIDI_OUTPUT TRUE
AU_MAIN_TYPE kAudioUnitType_MIDIProcessor
VST3_CATEGORIES Instrument|Generator
```

- Logic sees an AU MIDI processor (`aumi`) and inserts it like a MIDI FX.
- VST3 looks like a silent instrument that **outputs MIDI**. Reaper can still use it as MIDI FX. Ableton needs “MIDI To” from this track to the synth track. Document that in the plugin README; do not block v0 on Ableton.

Plugin names:

- Product: `OpenChord M Core`
- Manufacturer: `OpenChord`
- Four-char manufacturer code: `OpCh` (must have a capital).
- Unique plugin code: `OcMC` (not `OcM1` — that reads as hardware).

No audio needed musically. Keep a silent stereo output bus so VST3 hosts that require audio ports will load it. Output zeros.

License: MIT, same as the repo. JUCE has its own license — use the GPL/JUCE6+ AGPLv3 path **or** document that we need a JUCE license for closed distribution. For this open-source repo, JUCE under GPL/AGPL is OK if the plugin stays GPL-compatible **or** Fetch JUCE and keep the plugin GPL. **Prefer:** JUCE as a CMake FetchContent of the official repo; do not vendor a 200 MB tree if a fetch works. If JUCE GPL vs our MIT is a conflict, say so in `m1/plugin/README.md` and keep the plugin source MIT-intended with a note. Do not stall v0 on relicensing.

---

## How it should feel (Type mode)

Port the proto. Do not “improve” assignment.

Source of truth for feel: `m1/proto-rp2040/src/main.cpp`.

- Piano **notes** = roots (unless that note is mapped as a control).
- Type held at **note-on** assigns that key a chord until **that key** is released, even if the Type button is let go.
- No Type at note-on = **thru** (echo the note).
- New keys do not steal an already-held chord. Two Types can hold two chords.
- 6 / m7 / M7 / 9 do nothing without a Type. With a Type they add color (Maj + M7 = maj7).
- Hold Type + key, then tap an extra: add notes without retriggering the whole chord (`DiffVoicing` in the proto).
- MIDI out is **ref-counted** so thru E on top of Cmaj does not kill the chord’s E.
- Stick is two axes, center = HOME. `oc::SeatFromStick(x, y)`. Seat changes re-render sounding chords.
- Knobs that sit at 0: ignore until the CC passes 48–80 once (pickup), then track. Same as proto.
- Key: when the Key control is held, the next root note sets `key_pc` (0–11) and is **not** a chord/thru note.
- Panic: all notes off, clear voices, CC 123 optional.

UI names: **Type**. Not Orchid.

### Default map (Launchkey Mini MK4 37)

Matches the proto so the author’s rig works with zero Learn:

| Control | Default MIDI |
|---------|----------------|
| Dim Min Maj Sus | CC 36 37 38 39 |
| 6 m7 M7 9 | CC 40 41 42 43 |
| Stick X / Y | CC 47 / 48 |
| Pads “held” | CC value ≥ 64 |

Notes are roots. Do **not** ignore channel 10 notes unless we are on this default pad map and the event is a **note** on ch 10 (Launchkey pads are CC, not notes). Prefer: if a message matches a binding, it is a control; otherwise notes are roots.

Ship this as a named preset: `Launchkey Mini MK4`.

---

## MIDI Learn (the point of v0)

Every assignable control can be bound to **one** MIDI message: note, or CC (channel + number). Optional: aftertouch / program change not needed in v0.

Assignable in v0:

- Dim, Min, Maj, Sus
- 6, m7, M7, 9
- Stick X, Stick Y
- Key
- Panic (optional but cheap)
- Shift: show the button, leave unbound / unused (proto Shift is reserved)

### Gesture

1. User clicks the control in the plugin UI.
2. That control **arms** (flash / “move a control…”).
3. The next MIDI message that is a note-on (vel > 0) or a CC **latch** binds to it.
4. Arm clears. Binding is stored. Same message is **not** also treated as a root / stick move for that event.
5. Clicking a different control while armed moves the arm. Clicking the armed control again, or Escape, or a 8s timeout, cancels.

Stick axes: the next **CC** (not a note) binds that axis. Moving a knob while Stick X is armed maps that CC. Pickup still applies after bind.

If the user arms Maj and hits a piano key, Maj is now that note. That pitch is no longer a root; it is a Type pad. That is correct. They can unlearn.

### Unlearn / conflict

- Right-click or a small `x` on the control clears the binding.
- Binding the same MIDI to a second control **moves** it (one message → one control).
- Show the binding on the button: `Maj` / `CC 38` or `Maj` / `C#2`.

### Persistence

Store the map in plugin state (JUCE `getStateInformation` / `setStateInformation`) so Logic recalls it with the session. Include a **Reset to Launchkey** button.

Do not require a file picker for v0.

---

## UI (ugly, required)

One window, ~500×400, default LookAndFeel. No images.

```
OpenChord M Core                [Launchkey Mini MK4] [Reset map]
Key: C major     Chord: Cmaj7     Type

[ Dim ] [ Min ] [ Maj ] [ Sus ]
[  6  ] [ m7  ] [  M7 ] [  9  ]

[ Key ] [ Panic ]

Stick X --------O--------     (or two sliders)
Stick Y --------O--------

Learn: click a button, then press the pad / move the knob.
```

- Type / extra buttons are **momentary in MIDI** (held while CC/note held) and **click-and-hold** with the mouse so you can test with no controller.
- Mouse click on Dim without Learn = hold Dim until mouse up (same as a pad).
- Chord name from `Voicing.name` (engine already fills it) or a simple label from type+root.
- Armed control: invert colors or a `*` prefix.
- Stick sliders are display + mouse override. MIDI Learn still binds CCs. Moving the on-screen slider is enough to play without a stick.

No Degree toggle in v0. You may show a disabled “Degree (later)” so the UI matches the product, or omit it. Do not implement Degree.

---

## Architecture

```
m1/engine/          oc::Render, SeatFromStick. Compile this into the plugin.
                    Do not copy the .cpp into the plugin tree; add the engine
                    sources to the JUCE target.
m1/plugin/          JUCE project (CMake). Processor, editor, mapping, session.
m1/proto-rp2040/    Do not make the plugin include Arduino headers.
```

**Prefer** extracting the proto’s voice/refcount/pad logic into portable C++ next to the engine, e.g. `m1/engine/session.h` + `.cpp`:

- Inputs: note on/off, CC, Key held, panic, stick x/y
- Outputs: MIDI note on/off to send (channel, pitch, vel)
- Internals: the `Voice` array, type stack, `DiffVoicing`, extra refresh

Then proto and plugin both call the session. If extracting would delay a first loadable AU by days, duplicate the logic in the plugin **and** leave a `TODO: share session with proto`. Do not silently drift.

Plugin `processBlock`:

1. Read incoming MIDI.
2. If Learn is armed, maybe consume one event for the map.
3. If a message matches a binding, update Type / extra / stick / Key / panic.
4. Otherwise note on/off → session as roots.
5. Emit session MIDI to the output buffer.
6. Clear audio.

Realtime rule: no heap, no locks, no file IO on the audio thread. Learn arm flags are atomics. UI updates via `AsyncUpdater` or a timer (30 Hz) reading a snapshot (chord name, held pads, stick).

---

## Logic test (author’s rig)

This is the acceptance path.

1. Build AU. Logic should see **OpenChord M Core** as a MIDI effect / MIDI processor.
2. Instrument track (any synth). MIDI FX slot: OpenChord M Core.
3. Launchkey Mini MK4 → this track (not also to the synth).
4. Default map: hold Maj pad, play C → C major from the synth. No Type → raw key.
5. Low keys (C1–G1) chord. High keys stay in that octave (engine already follows the played root).
6. Click **Min** in the plugin (Learn), hit a different pad, that pad is now Min.
7. Knobs at 0 do not yank voicing southwest until swept through center.

VST3: load in Reaper (or Ableton with MIDI routed out of the plugin track). Same Learn. Do not block on Ableton if Logic AU + Reaper VST3 work.

---

## Repo / build

- Code under `m1/plugin/`. CMakeLists there. Engine files listed as target sources (`../engine/chord_engine.cpp`).
- Document build in `m1/plugin/README.md`: macOS, CMake, Xcode or Ninja, copy AU to `~/Library/Audio/Plug-Ins/Components`, VST3 to `~/Library/Audio/Plug-Ins/VST3`.
- `COPY_PLUGIN_AFTER_BUILD TRUE` on Mac is helpful.
- Do not add the plugin to `archive/` or `s1/`.
- Do not commit JUCE binaries or built `.vst3` / `.component` if they are huge; commit source + CMake. A `*.vst3` in gitignore is fine.

---

## Success checklist

- [ ] Logic loads OpenChord M Core as AU MIDI FX.
- [ ] VST3 builds from the same target.
- [ ] Maj + C key = C major; no Type = thru.
- [ ] Live extra (Maj held, tap M7) adds notes without killing the triad.
- [ ] MIDI Learn: click control, send MIDI, binding sticks and recalls with the project.
- [ ] Reset map restores Launchkey CC 36–43 / 47 / 48.
- [ ] Mouse can hold Type buttons and move stick sliders.
- [ ] UI shows current chord name and key.
- [ ] No Orchid / HiChord strings in the UI.
- [ ] `archive/s1-daisy/` untouched.

---

## Reading order for the implementer

1. This file.
2. [goals.md](goals.md) — plugin section and locked list.
3. [chord-engine.md](chord-engine.md) — Type mode only for v0.
4. `m1/engine/chord_engine.h` + `.cpp`
5. `m1/proto-rp2040/src/main.cpp` + `m1/proto-rp2040/README.md`

When in doubt, match the proto, then add Learn and an ugly editor.
