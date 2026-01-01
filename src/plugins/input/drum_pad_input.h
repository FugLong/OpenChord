#pragma once

#include "../../core/plugin_interface.h"
#include "../../core/midi/midi_types.h"
#include "../../core/io/button_input_handler.h"
#include <vector>

namespace OpenChord {

// Forward declaration
class InputManager;

/**
 * Drum Pad Input Plugin
 * 
 * Maps buttons 1-7 to drum/percussion MIDI notes.
 * Uses standard GM (General MIDI) drum mapping on channel 9 (channel 10 in 1-based).
 * Each button triggers a different drum sound.
 * 
 * Button mapping (default GM drum kit):
 * - Button 0 (White0): Kick (C1 = MIDI 36)
 * - Button 1 (White1): Snare (D1 = MIDI 38)
 * - Button 2 (White2): Hi-Hat Closed (E1 = MIDI 40)
 * - Button 3 (White3): Hi-Hat Open (F1 = MIDI 42)
 * - Button 4 (Black0): Crash Cymbal (C#1 = MIDI 37)
 * - Button 5 (Black1): Ride Cymbal (D#1 = MIDI 39)
 * - Button 6 (Black2): Tom (F#1 = MIDI 41)
 */
class DrumPadInput : public IInputPlugin {
public:
    DrumPadInput();
    ~DrumPadInput();
    
    // IPlugin interface
    void Init() override;
    void Process(float* in, float* out, size_t size) override;
    void Update() override;
    void UpdateUI() override;
    void HandleEncoder(int encoder, float delta) override;
    void HandleButton(int button, bool pressed) override;
    void HandleJoystick(float x, float y) override;
    
    const char* GetName() const override { return "Drum Pad"; }
    const char* GetCategory() const override { return "Input"; }
    int GetVersion() const override { return 1; }
    bool IsExclusive() const override { return true; }  // Exclusive: deactivates other exclusive plugins
    
    void SaveState(void* buffer, size_t* size) const override;
    void LoadState(const void* buffer, size_t size) override;
    size_t GetStateSize() const override;
    
    // IInputPlugin interface
    void GenerateMIDI(MidiEvent* events, size_t* count, size_t max_events) override;
    void ProcessMIDI(const MidiEvent* events, size_t count) override;
    bool IsActive() const override { return active_; }
    void SetActive(bool active) override { active_ = active; }
    int GetPriority() const override { return 40; }  // Higher priority than chromatic, lower than chord mapping
    
    // Setup - must be called before Init()
    void SetInputManager(InputManager* input_manager) {
        input_manager_ = input_manager;
    }
    
    // Get currently active drum notes (for UI display)
    std::vector<uint8_t> GetActiveNotes() const;
    
private:
    InputManager* input_manager_;
    bool active_;
    bool initialized_;
    
    // Button state tracking
    bool prev_button_states_[7];
    bool current_button_states_[7];
    
    // MIDI event buffer
    std::vector<MidiEvent> pending_events_;
    size_t pending_read_pos_;
    size_t pending_write_pos_;
    
    // Drum pad mapping
    static constexpr uint8_t DRUM_CHANNEL = 9;  // MIDI channel 10 (0-based = 9)
    
    void ProcessButtons();
    uint8_t GetDrumNote(int button_index) const;
};

} // namespace OpenChord



