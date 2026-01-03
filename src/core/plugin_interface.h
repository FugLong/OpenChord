#pragma once

#include <cstddef>
#include <cstdint>
#include "midi/midi_types.h"

namespace OpenChord {

/**
 * Base interface for all plugins in the OpenChord system
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;

    // Core lifecycle
    virtual void Init() = 0;
    virtual void Process(const float* const* in, float* const* out, size_t size) = 0;
    virtual void Update() = 0;  // Called every audio block for non-audio processing

    // UI and control handling
    virtual void UpdateUI() = 0;
    virtual void HandleEncoder(int encoder, float delta) = 0;
    virtual void HandleButton(int button, bool pressed) = 0;
    virtual void HandleJoystick(float x, float y) = 0;

    // Plugin metadata
    virtual const char* GetName() const = 0;
    virtual const char* GetCategory() const = 0;
    virtual int GetVersion() const = 0;
    
    // Exclusive plugin behavior
    // If a plugin is exclusive, activating it will deactivate all other exclusive plugins
    // This is useful for input modes that cannot coexist (e.g., chord mapping and drum pad)
    virtual bool IsExclusive() const { return false; }

    // State management
    virtual void SaveState(void* buffer, size_t* size) const = 0;
    virtual void LoadState(const void* buffer, size_t size) = 0;
    virtual size_t GetStateSize() const = 0;
};

/**
 * Interface for Input Stack plugins that generate or transform MIDI
 */
class IInputPlugin : public IPlugin {
public:
    // MIDI generation/transformation
    virtual void GenerateMIDI(MidiEvent* events, size_t* count, size_t max_events) = 0;
    virtual void ProcessMIDI(const MidiEvent* events, size_t count) = 0;
    
    // Input stack specific
    virtual bool IsActive() const = 0;
    virtual void SetActive(bool active) = 0;
    virtual int GetPriority() const = 0;  // For stack ordering
};

/**
 * Interface for Instrument plugins that generate audio from MIDI
 */
class IInstrumentPlugin : public IPlugin {
public:
    // MIDI handling
    virtual void NoteOn(int note, float velocity) = 0;
    virtual void NoteOff(int note) = 0;
    virtual void AllNotesOff() = 0;
    virtual void SetPitchBend(float bend) = 0;
    virtual void SetModulation(float mod) = 0;
    
    // Polyphony
    virtual int GetMaxPolyphony() const = 0;
    virtual int GetActiveVoices() const = 0;
};

/**
 * Interface for Effects plugins that process audio
 */
class IEffectPlugin : public IPlugin {
public:
    // Audio processing
    virtual void SetSampleRate(float sample_rate) = 0;
    virtual void SetBypass(bool bypass) = 0;
    virtual bool IsBypassed() const = 0;
    
    // Effect-specific
    virtual void SetWetDry(float wet_dry) = 0;
    virtual float GetWetDry() const = 0;
};

/**
 * Interface for PlayMode plugins that override system behavior
 */
class IPlayModePlugin : public IPlugin {
public:
    // PlayMode specific
    virtual void EnterMode() = 0;
    virtual void ExitMode() = 0;
    virtual bool IsActive() const = 0;
    
    // Override system controls
    virtual bool OverrideEncoder(int encoder, float delta) = 0;
    virtual bool OverrideButton(int button, bool pressed) = 0;
    virtual bool OverrideJoystick(float x, float y) = 0;
    
    // UI override
    virtual bool OverrideUI() = 0;
    virtual void RenderCustomUI() = 0;
};

} // namespace OpenChord 