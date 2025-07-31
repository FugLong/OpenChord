# OpenChord Firmware Architecture

## Core Concepts

* Portable, open-source music device using the Daisy Seed MCU
* Built-in sequencer, looper, sampler, instruments, audio effects, and expressive performance features
* Highly modular firmware with plugin-like components (bundled at compile time)
* Tracks are the primary unit of music creation. Scenes allow dynamic performance and composition by storing variations in track loop content and parameters, while the tracks themselves define the fixed signal/control path.

## Track Structure

Each track consists of modular stages:

### 1. Input Stack
One or more modules that generate or transform MIDI. This includes:
* **Recorded MIDI loops** (handled by the OS's internal looper, appear at the top of the stack)
* **Core MIDI inputs** from keypad, USB, or 3.5mm MIDI (always present below recorded loops)
* **Step sequencers**
* **MIDI/Joystick remapping tools** (e.g. chord or scale mapping)
* **MIDI effects or generators** (e.g. randomizers, arpeggiators)

### 2. Instrument
The core sound generator, e.g. a synth engine or sampler.
* Built-in options provided
* Modular engine support through firmware-bundled plugins

### 3. Effects
Post-processing like delay, reverb, filters, saturation
* Effects are per-track

## Scenes

* Scenes are closely integrated with the internal loop recorder system
* Scenes allow the user to snapshot and switch between variations of the current track
* Each scene stores:
  * The state of recorded MIDI loops (and optionally audio loops)
  * Parameter states of the currently loaded Instruments and Effects
* Scenes do **not** change:
  * The types or layout of Input Stack, Instrument, or FX modules — only their internal parameter values and loop content
* Only Scene 1 can change which modules are loaded into memory
* Spawning a new Scene creates an alternate version of the track's musical state and parameters
* Scene switching can be manual or sequenced
* Removing a plugin in Scene 1 removes it globally

## UI and Interaction

* **Track Selector**: Choose which track is currently focused for editing/play
* **Tabs or Pages** (Categorized for clarity):
  * **Input**: Edit MIDI generators/effects and recorded loops
  * **Instrument**: Choose or edit the instrument module
  * **FX**: Tweak post-processing effects
* Potentially support for custom UIs driven by plugins being interacted with.
  * A main outer/top level UI would always be present around the border or at the top of the screen
  * But a plugin could request to drive an inner section of screen manually.
  * This allows plugins to have as advanced or simple UIs as they want.

## PlayModes

A PlayMode is a global system behavior mode. It changes how the device and controls operate, suspending normal UI flow for a specialized interaction context. Examples:
* **Performance FX Mode**: Keys mapped to trigger effect actions (e.g. stutter, reverse, pitch shift)
* **Scene Switcher Mode**: Use buttons or pads to change scenes live

PlayModes can:
* Specify their own control mappings, overlays, and interface logic
* Use effects, but these are configured by the PlayMode designer, not user-selected
* Temporarily override the default system interaction model

**Note**: Features like chord/note input mapping should be implemented as input plugins, not PlayModes.

## Plugin System

* Plugins are modular code components for Inputs, Instruments, Effects, or PlayModes
* They are bundled at compile time — no dynamic loading
* Community-developed plugins are added via firmware PRs
* Categorized selection improves UX when browsing large collections
* Unified interface for all plugin types

## Goals

* Empower rapid music sketching and performance anywhere
* Maximize expression with minimal controls (joystick + encoder + keys)
* Support creative expansion via plugin ecosystem (bundled with firmware)
* Maintain modularity, readability, and clean UX at all levels 