# OpenChord M Core (plugin)

Free AU MIDI FX + VST3. Same chord engine / session as the **OpenChord M1** hardware. MIDI in → chords out. No audio.

| Name | What |
|------|------|
| **OpenChord M1** | Hardware box (USB MIDI device) |
| **OpenChord M Core** | This free plugin |

**Spec:** [`../docs/plugin.md`](../docs/plugin.md)

## License note

Plugin source is MIT-intended like the rest of the repo. JUCE is fetched via CMake and is GPL/AGPLv3 unless you have a commercial JUCE license. Distributing a binary built this way requires GPL-compatible terms for that binary (or a JUCE license). Source here stays usable either way.

## Build (macOS)

Needs CMake ≥ 3.22, a C++17 compiler, and network once to FetchContent JUCE 8.

```bash
cd m1/plugin
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Xcode generator also works: `-G Xcode`.

`COPY_PLUGIN_AFTER_BUILD` installs into:

| Format | Path |
|--------|------|
| AU | `~/Library/Audio/Plug-Ins/Components/OpenChord M Core.component` |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/OpenChord M Core.vst3` |

Standalone: `build/OpenChordMCore_artefacts/Release/Standalone/OpenChord M Core.app` (path may vary by generator).

Rescan plugins in Logic / Reaper after the first build. If you previously installed an older **OpenChord M1** plugin bundle, remove it from those folders so only M Core remains.

## Logic (AU MIDI FX)

1. Instrument track → any synth.
2. MIDI FX slot → **OpenChord M Core**.
3. Route Launchkey (or other controller) to this track only.
4. Default map = Launchkey Mini MK4 (pads CC 36–43, stick CC 47/48). Hold Maj, play C → C major. No Type → thru.

## VST3 (Reaper / Ableton / etc.)

VST3 has no MIDI-FX type. This plugin is a silent instrument that outputs MIDI.

- **Reaper:** can use it as MIDI FX on a track.
- **Ableton:** put OpenChord M Core on a MIDI track, set the synth track’s MIDI From to that track (“MIDI To” routing). Do not expect an AU-style MIDI FX slot.

## MIDI Learn

Click a control (short click), then press the pad or move the knob. Binding stores in plugin state with the session. Hold a button to play Type/extra with the mouse. Right-click clears. **Reset map** restores Launchkey defaults. Esc cancels learn.

## Layout

```
m1/plugin/          This JUCE project (OpenChord M Core)
m1/engine/          chord_engine + session (compiled into the plugin; shared with M1 hardware)
```
