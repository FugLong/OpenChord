#pragma once

#include "debug_view.h"
#include "../io/display_manager.h"
#include "../io/io_manager.h"
#include "../io/input_manager.h"
#include "../audio/audio_engine.h"
#include "../audio/volume_manager.h"
#include "../midi/midi_handler.h"

namespace OpenChord {

/**
 * Helper functions to create common debug views
 * These make it easy to add debug info without writing custom render functions
 */

// System status view - shows IO manager health
void RenderSystemStatus(DisplayManager* display, IOManager* io_manager);

// Input view - shows encoder, joystick values (needs both input_manager and io_manager for raw values)
void RenderInputStatus(DisplayManager* display, InputManager* input_manager, IOManager* io_manager);

// Analog view - shows volume, joystick, mic, battery
void RenderAnalogStatus(DisplayManager* display, IOManager* io_manager);

// Audio view - shows audio engine state
void RenderAudioStatus(DisplayManager* display, AudioEngine* audio_engine, VolumeManager* volume_manager);

// MIDI view - shows MIDI interface status
void RenderMIDIStatus(DisplayManager* display, OpenChordMidiHandler* midi_handler);

} // namespace OpenChord