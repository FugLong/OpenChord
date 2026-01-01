#pragma once

#include "midi/midi_handler.h"
#include "ui/global_settings.h"
#include <cstdint>

namespace OpenChord {

/**
 * Transport Control - Handles MIDI transport commands
 * 
 * Button mappings (handled in main.cpp):
 * - INPUT tap = Play/Pause toggle
 * - RECORD tap = Record toggle
 * - INPUT hold = Input Stack menu (handled in main.cpp)
 * - RECORD hold = Global Settings menu (handled in main.cpp)
 */
class TransportControl {
public:
    TransportControl();
    ~TransportControl();
    
    // Initialization
    void Init(OpenChordMidiHandler* midi_handler, GlobalSettings* global_settings);
    
    // Update (call from main loop)
    // Called when button combo is detected (on release)
    // combo_type: 0 = INPUT+RECORD (play/pause), 1 = RECORD alone (record toggle)
    void HandleCombo(int combo_type);
    
    // Get current transport state (for UI feedback)
    bool IsPlaying() const { return is_playing_; }
    bool IsRecording() const { return is_recording_; }
    
private:
    OpenChordMidiHandler* midi_handler_;
    GlobalSettings* global_settings_;
    
    // Transport state
    bool is_playing_;
    bool is_recording_;
    
    // Send transport commands based on routing setting
    void SendPlayPause();
    void SendRecord();
    void SendContinue();
};

} // namespace OpenChord

