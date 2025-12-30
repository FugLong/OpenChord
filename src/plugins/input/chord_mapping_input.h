#pragma once

#include "../../core/plugin_interface.h"
#include "../../core/midi/midi_types.h"
#include "../../core/music/chord_engine.h"
#include "../../core/io/button_input_handler.h"
#include "../../core/io/joystick_input_handler.h"  // For JoystickDirection enum
#include <vector>

// Forward declaration
namespace OpenChord {
    class InputManager;
}

namespace OpenChord {

/**
 * Chord Mapping Input Plugin
 * 
 * Maps button presses (1-7) to chords and uses joystick to modify
 * chord inversions and extensions. Generates MIDI events for all
 * notes in the chord.
 */
class ChordMappingInput : public IInputPlugin {
public:
    ChordMappingInput();
    ~ChordMappingInput();
    
    // IPlugin interface
    void Init() override;
    void Process(float* in, float* out, size_t size) override;
    void Update() override;
    void UpdateUI() override;
    void HandleEncoder(int encoder, float delta) override;
    void HandleButton(int button, bool pressed) override;
    void HandleJoystick(float x, float y) override;
    
    const char* GetName() const override { return "Chord Mapping"; }
    const char* GetCategory() const override { return "Input"; }
    int GetVersion() const override { return 1; }
    
    void SaveState(void* buffer, size_t* size) const override;
    void LoadState(const void* buffer, size_t size) override;
    size_t GetStateSize() const override;
    
    // IInputPlugin interface
    void GenerateMIDI(MidiEvent* events, size_t* count, size_t max_events) override;
    void ProcessMIDI(const MidiEvent* events, size_t count) override;
    bool IsActive() const override { return active_; }
    void SetActive(bool active) override { active_ = active; }
    int GetPriority() const override { return 50; }  // Medium priority
    
    // Setup - must be called before Init()
    void SetInputManager(InputManager* input_manager) {
        input_manager_ = input_manager;
    }
    
    // Get current chord for UI display
    const Chord* GetCurrentChord() const {
        return current_chord_.note_count > 0 ? &current_chord_ : nullptr;
    }
    
    // Key selection
    void SetKey(MusicalKey key);
    MusicalKey GetCurrentKey() const { return current_key_; }
    
    // Joystick preset management
    void SetJoystickPreset(int preset_index);
    int GetCurrentJoystickPreset() const { return current_joystick_preset_index_; }
    
    // Get current joystick direction
    JoystickDirection GetCurrentJoystickDirection() const { return current_joystick_direction_; }
    
private:
    // Input access
    InputManager* input_manager_;
    
    // State
    bool active_;
    bool initialized_;
    
    // Chord engine
    ChordEngine chord_engine_;
    
    // Current chord state
    Chord current_chord_;
    bool chord_active_;
    
    // Key and preset
    MusicalKey current_key_;
    int current_joystick_preset_index_;
    const JoystickPreset* current_joystick_preset_;
    
    // Button state tracking
    bool prev_button_states_[7];  // One for each musical button
    
    // Joystick state
    float joystick_x_;
    float joystick_y_;
    JoystickDirection current_joystick_direction_;
    JoystickDirection prev_joystick_direction_;
    
    // MIDI event buffer
    std::vector<MidiEvent> pending_events_;
    size_t pending_read_pos_;
    size_t pending_write_pos_;
    
    // Helper methods
    void ProcessButtons();
    void ProcessJoystick();
    void UpdateChord(int button_index);
    JoystickDirection GetJoystickDirectionFromXY(float x, float y) const;
    
    // Convert button index (0-6) to MusicalButton enum
    MusicalButton ButtonIndexToMusicalButton(int index) const;
};

} // namespace OpenChord

