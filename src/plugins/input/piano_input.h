#pragma once

#include "../../core/plugin_interface.h"
#include "../../core/midi/midi_types.h"
#include "../../core/io/button_input_handler.h"
#include "../../core/music/chord_engine.h"
#include "../../core/ui/plugin_settings.h"
#include <vector>

namespace OpenChord {

// Forward declaration
class InputManager;
class OctaveShift;
class Track;

/**
 * Piano Input Plugin - Default piano/keyboard input mode
 * 
 * Supports two modes:
 * - Chromatic: Maps buttons 1-7 chromatically (white and black keys)
 * - Scale: Maps buttons 1-7 to I-VII scale degrees in a selected key
 * 
 * Physical layout: White0, White1, White2, White3, Black0, Black1, Black2
 * In chromatic mode: C, D, E, F, C#, D#, F# (default starting from C4)
 * In scale mode: I, II, III, IV, V, VI, VII (scale degrees in selected key)
 */
class PianoInput : public IInputPlugin, public IPluginWithSettings {
public:
    enum class PlayMode {
        CHROMATIC = 0,
        SCALE = 1
    };
    
    PianoInput();
    ~PianoInput();
    
    // IPlugin interface
    void Init() override;
    void Process(float* in, float* out, size_t size) override;
    void Update() override;
    void UpdateUI() override;
    void HandleEncoder(int encoder, float delta) override;
    void HandleButton(int button, bool pressed) override;
    void HandleJoystick(float x, float y) override;
    
    const char* GetName() const override { return "Notes"; }
    const char* GetCategory() const override { return "Input"; }
    int GetVersion() const override { return 2; }
    bool IsExclusive() const override { return true; }  // Exclusive: deactivates other exclusive plugins
    
    void SaveState(void* buffer, size_t* size) const override;
    void LoadState(const void* buffer, size_t size) override;
    size_t GetStateSize() const override;
    
    // IInputPlugin interface
    void GenerateMIDI(MidiEvent* events, size_t* count, size_t max_events) override;
    void ProcessMIDI(const MidiEvent* events, size_t count) override;
    bool IsActive() const override;
    void SetActive(bool active) override { active_ = active; }
    int GetPriority() const override { return 10; }  // Highest priority (lowest number) - appears first
    
    // Set track to check for other active plugins and ensure default activation
    void SetTrack(Track* track) { track_ = track; }
    
    // Setup
    void SetInputManager(InputManager* input_manager) {
        input_manager_ = input_manager;
    }
    
    void SetOctaveShift(OctaveShift* octave_shift) {
        octave_shift_ = octave_shift;
    }
    
    // Get currently active notes (for UI display)
    std::vector<uint8_t> GetActiveNotes() const;
    
    // Key selection
    void SetKey(MusicalKey key);
    MusicalKey GetCurrentKey() const { return current_key_; }
    
    // Play mode selection
    void SetPlayMode(PlayMode mode);
    PlayMode GetCurrentPlayMode() const { return play_mode_; }
    
    // IPluginWithSettings interface
    int GetSettingCount() const override;
    const PluginSetting* GetSetting(int index) const override;
    void OnSettingChanged(int setting_index) override;
    
private:
    InputManager* input_manager_;
    OctaveShift* octave_shift_;
    Track* track_;  // To check for other active plugins and ensure default activation
    bool active_;
    bool initialized_;
    
    // Chord engine for scale mode
    ChordEngine chord_engine_;
    
    // Play mode and key
    PlayMode play_mode_;
    MusicalKey current_key_;
    
    // Settings support
    static constexpr int SETTING_COUNT = 3;  // Mode, Key root, Mode (scale mode)
    mutable PluginSetting settings_[SETTING_COUNT];
    mutable int mode_setting_value_;  // Helper for mode enum setting
    mutable int play_mode_setting_value_;  // Helper for play mode enum setting
    mutable int key_root_setting_value_;  // Helper for key root enum setting
    void InitializeSettings();
    
    // Button state tracking
    bool prev_button_states_[7];
    bool current_button_states_[7];  // Track current pressed state
    
    // MIDI event buffer
    std::vector<MidiEvent> pending_events_;
    size_t pending_read_pos_;
    size_t pending_write_pos_;
    
    // Joystick state for pitch bend and mod wheel
    float joystick_x_;
    float joystick_y_;
    int16_t last_pitch_bend_value_;
    uint8_t last_mod_wheel_value_;
    
    void ProcessButtons();
    void ProcessJoystick();
    uint8_t GetMidiNote(int button_index) const;
    uint8_t GetScaleNote(int button_index) const;  // For scale mode
    int16_t CalculatePitchBend(float joystick_y) const;
    uint8_t CalculateModWheel(float joystick_x) const;
    
    // Ensure default activation when all plugins are off
    void EnsureDefaultActivation();
};

} // namespace OpenChord

