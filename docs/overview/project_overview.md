# OpenChord - Portable Open-Source Music Device

## Project Overview

My goal is to design and build a powerful, portable, and open-source music creation device with a workflow that just makes sense. It features hands-on control, beautiful sound, MIDI I/O, looper capabilities, and a sturdy 3D-printable enclosure, making it perfect for spontaneous music-making anywhere. Modular firmware architecture supports community-driven plugin development, creating an expandable ecosystem for artists and developers.

## 🔥 Use Cases

* 🎹 **Chord Generator**: Use joystick + chord mapping for fast harmonic inspiration
* 🎛️ **Synthesizer**: Design, tweak, and perform complex patches
* 🔁 **Looper**: Capture and layer performance ideas
* 🥁 **Sampler / Drum Machine**: Trigger from built-in pads or MIDI
* 🎚️ **DAW Companion**: Send and receive MIDI, control software instruments
* 🎼 **TRS MIDI Host and Client**: Play with your favorite controller or control your favorite hardware synth

## 🎧 Key Features

* **Standalone Synth Engine** with high-quality, expressive presets ("analog", granular, wavetable, fm, all of it)
* **Advanced Sound Design Options** accessible via encoders and menus
* **Powerful effects** like reverb, delay, etc. and even live performance effects
* **Drum(+More) Sampler** for beat-making (great presets, easy to upload packs)
* **Microphone and/or line in** for sampling and recording
* **Feature-Rich Looper** with layering(maybe...), overdub, free length
* **MIDI Controller Mode** to use as input for your DAW
* **MIDI Keyboard Input** for velocity-sensitive, expressive play
* **Battery Powered**, compact, and durable for true portability
* **Beautiful, Intuitive UI** on a sharp OLED screen with joystick and scroll wheel navigation

## 🧩 System Architecture

| Subsystem | Tech |
|-----------|------|
| MCU + Audio | Daisy Seed + libDaisy |
| UI | OLED screen + joystick + encoders + keys |
| Looper | Midi based, time-stretch, sync |
| Synth Engine | Wavetable, FM, poly/mono modes |
| Sampler | SD-based samples, mapped pads |
| Storage | microSD for loops, patches, and samples |
| Power | USB-C + PowerBoost 1000C + LiPo battery |
| Audio | headphone out, line in, 3.5mm TRS |
| MIDI I/O | 3.5mm TRS and USB MIDI (host + device) |

## 🖨️ Enclosure Plan

* 3D-printed, designed in Onshape
* Compact, durable, travel-ready
* Layout supports screen, encoders, joystick, keys
* Top ports for SD card, jacks