#pragma once

#include "../../core/plugin_interface.h"
#include "../../core/midi/midi_types.h"
#include "../../core/io/button_input_handler.h"
#include <vector>

namespace OpenChord {

// Forward declaration
class InputManager;
class OctaveShift;
class Track;

/**
 * Chromatic Input Plugin - Default chromatic key mapping
 * 
 * Maps buttons 1-7 chromatically (white and black keys).
 * Used when chord mapping is inactive.
 * Physical layout: White0, White1, White2, White3, Black0, Black1, Black2
 * Maps to: C, D, E, F, C#, D#, F# (default starting from C4)
 */
class ChromaticInput : public IInputPlugin {
public:
    ChromaticInput();
    ~ChromaticInput();
    
    // IPlugin interface
    void Init() override;
    void Process(float* in, float* out, size_t size) override;
    void Update() override;
    void UpdateUI() override;
    void HandleEncoder(int encoder, float delta) override;
    void HandleButton(int button, bool pressed) override;
    void HandleJoystick(float x, float y) override;
    
    const char* GetName() const override { return "Chromatic"; }
    const char* GetCategory() const override { return "Input"; }
    int GetVersion() const override { return 1; }
    
    void SaveState(void* buffer, size_t* size) const override;
    void LoadState(const void* buffer, size_t size) override;
    size_t GetStateSize() const override;
    
    // IInputPlugin interface
    void GenerateMIDI(MidiEvent* events, size_t* count, size_t max_events) override;
    void ProcessMIDI(const MidiEvent* events, size_t count) override;
    bool IsActive() const override;
    void SetActive(bool active) override { active_ = active; }
    int GetPriority() const override { return 100; }  // Lower priority than chord mapping
    
    // Set track to check for other active plugins
    void SetTrack(const Track* track) { track_ = track; }
    
    // Setup
    void SetInputManager(InputManager* input_manager) {
        input_manager_ = input_manager;
    }
    
    void SetOctaveShift(OctaveShift* octave_shift) {
        octave_shift_ = octave_shift;
    }
    
    // Get currently active notes (for UI display)
    std::vector<uint8_t> GetActiveNotes() const;
    
private:
    InputManager* input_manager_;
    OctaveShift* octave_shift_;
    const Track* track_;  // To check if other plugins are active
    bool active_;
    bool initialized_;
    
    // Button state tracking
    bool prev_button_states_[7];
    bool current_button_states_[7];  // Track current pressed state
    
    // MIDI event buffer
    std::vector<MidiEvent> pending_events_;
    size_t pending_read_pos_;
    size_t pending_write_pos_;
    
    void ProcessButtons();
    uint8_t GetMidiNote(int button_index) const;
};

} // namespace OpenChord

