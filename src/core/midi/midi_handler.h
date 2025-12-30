#pragma once

#include "daisy_seed.h"
#include "hid/midi.h"
#include "midi_interface.h"
#include "../audio/audio_engine.h"

namespace OpenChord {

/**
 * Unified MIDI handler for both USB and TRS MIDI
 * Handles all MIDI input/output and routes to the global MidiHub
 */
class OpenChordMidiHandler {
public:
    OpenChordMidiHandler();
    ~OpenChordMidiHandler();
    
    // Initialization
    void Init(daisy::DaisySeed* hw);
    
    // MIDI processing (called by system)
    void ProcessMidi(AudioEngine* audio_engine);
    
    // Status queries
    bool IsUsbInitialized() const { return usb_midi_initialized_; }
    bool IsTrsInitialized() const { return trs_midi_initialized_; }
    
    // MIDI output
    void SendMidi(const MidiHubEvent& event);
    void SendMidi(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1);
    
private:
    // USB MIDI
    daisy::MidiUsbHandler usb_midi_;
    bool usb_midi_initialized_;
    
    // TRS MIDI - using MidiUartHandler as per official Daisy Seed example
    daisy::MidiUartHandler trs_midi_;
    bool trs_midi_initialized_;
    
    // Hardware reference
    daisy::DaisySeed* hw_;
    
    // MIDI processing methods
    void ProcessUsbMidi();
    void ProcessTrsMidi();
    
    // Convert OpenChord MidiHubEvent to raw MIDI bytes
    void ConvertToMidiBytes(const MidiHubEvent& event, uint8_t* bytes, size_t* size);
    
    // Add events to MidiHub
    void AddToMidiHub(const daisy::MidiEvent& event, MidiHubEvent::Source source);
};

} // namespace OpenChord
