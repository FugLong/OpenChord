#pragma once

#include "system_interface.h"
#include "midi/midi_handler.h"
#include "midi/octave_shift.h"
#include "midi/midi_types.h"
#include "midi/midi_interface.h"
#include "tracks/track_interface.h"
#include <cstddef>

namespace OpenChord {

// Forward declarations
class OpenChordSystem;
class OpenChordMidiHandler;
class OctaveShift;
class Track;

/**
 * MIDI Router
 * 
 * Handles routing of MIDI events between:
 * - External MIDI inputs (USB, TRS) -> Tracks
 * - Tracks -> MIDI outputs (USB, TRS)
 * 
 * Consolidates the complex MIDI routing logic from main.cpp
 * Uses MidiHub for event storage and OpenChordMidiHandler for I/O
 */
class MidiRouter {
public:
    MidiRouter();
    ~MidiRouter();
    
    // Initialization
    void Init(OpenChordSystem* system, OpenChordMidiHandler* midi_handler,
             OctaveShift* octave_shift);
    
    /**
     * Route MIDI events
     * Call this from the main loop to:
     * 1. Route external MIDI input to tracks
     * 2. Route track-generated MIDI to outputs
     */
    void RouteMIDI();
    
private:
    OpenChordSystem* system_;
    OpenChordMidiHandler* midi_handler_;
    OctaveShift* octave_shift_;
    
    // Event buffers (reused to avoid allocations)
    static constexpr size_t MAX_EVENTS = 64;
    MidiEvent track_events_[MAX_EVENTS];
    MidiEvent generated_events_[MAX_EVENTS];
    
    // Conversion helpers
    MidiEvent ConvertHubToTrackEvent(const MidiHubEvent& hub_event);
    daisy::MidiMessageType ConvertTrackEventToHubType(const MidiEvent& track_event);
    
    // Routing methods
    void RouteExternalMIDI();
    void RouteGeneratedMIDI();
    bool IsBasicMidiInputPlugin(const char* plugin_name) const;
};

} // namespace OpenChord
