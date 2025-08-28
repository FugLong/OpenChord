#pragma once

#include "../../../core/plugin_interface.h"
#include "../../../core/midi/midi_types.h"
#include <vector>

namespace OpenChord {

/**
 * Basic MIDI input plugin that passes through external MIDI
 */
class BasicMidiInput : public IInputPlugin {
public:
    BasicMidiInput();
    ~BasicMidiInput();

    // IPlugin interface
    void Init() override;
    void Process(float* in, float* out, size_t size) override;
    void Update() override;
    void UpdateUI() override;
    void HandleEncoder(int encoder, float delta) override;
    void HandleButton(int button, bool pressed) override;
    void HandleJoystick(float x, float y) override;
    
    const char* GetName() const override { return "MIDI Input"; }
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
    int GetPriority() const override { return 100; }  // High priority for external MIDI

private:
    bool active_;
    std::vector<MidiEvent> midi_buffer_;
    size_t buffer_read_pos_;
    size_t buffer_write_pos_;
};

} // namespace OpenChord 