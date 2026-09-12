#pragma once

#include "../plugin_interface.h"
#include "../midi/midi_types.h"
#include "../music/chord_engine.h"
#include <vector>
#include <memory>

namespace OpenChord {

class OctaveShift;

/**
 * Track context - shared data accessible to all plugins
 */
struct TrackContext {
    MusicalKey key;         // Current musical key for the track
    float bpm;              // Current BPM (future use)
    
    TrackContext() : key(MusicalKey(0, MusicalMode::IONIAN)), bpm(120.0f) {}
};

/**
 * Track structure - the primary unit of music creation
 */
class Track {
public:
    enum class Focus {
        INPUT,
        INSTRUMENT,
        FX
    };

    Track();
    ~Track();

    // Track lifecycle
    void Init();
    void Process(const float* const* in, float* const* out, size_t size);
    void Update();

    // Input Stack management
    void AddInputPlugin(std::unique_ptr<IInputPlugin> plugin);
    void RemoveInputPlugin(size_t index);
    void ReorderInputPlugin(size_t from, size_t to);
    const std::vector<std::unique_ptr<IInputPlugin>>& GetInputPlugins() const;
    
    // Exclusive plugin management
    // When activating an exclusive plugin, deactivate all other exclusive plugins
    void SetInputPluginActive(IInputPlugin* plugin, bool active);

    // Instrument management
    void SetInstrument(std::unique_ptr<IInstrumentPlugin> instrument);
    IInstrumentPlugin* GetInstrument() const;
    void SetInstrumentEnabled(bool enabled);
    bool IsInstrumentEnabled() const;

    // Effects management
    void AddEffect(std::unique_ptr<IEffectPlugin> effect);
    void RemoveEffect(size_t index);
    void ReorderEffect(size_t from, size_t to);
    const std::vector<std::unique_ptr<IEffectPlugin>>& GetEffects() const;

    // Focus and UI
    void SetFocus(Focus focus);
    Focus GetFocus() const;
    void UpdateUI();
    void HandleEncoder(int encoder, float delta);
    void HandleButton(int button, bool pressed);
    void HandleJoystick(float x, float y);

    // MIDI processing
    void ProcessMIDI(const MidiEvent* events, size_t count);
    void GenerateMIDI(MidiEvent* events, size_t* count, size_t max_events);

    // Track state
    void SetMute(bool mute);
    bool IsMuted() const;
    void SetSolo(bool solo);
    bool IsSoloed() const;
    void SetName(const char* name);
    const char* GetName() const;

    // Track context (key, BPM, etc.) - accessible to plugins
    void SetKey(MusicalKey key);
    MusicalKey GetKey() const;
    const TrackContext& GetContext() const { return context_; }

    // Octave shift
    void SetOctaveShift(OctaveShift* octave_shift) { octave_shift_ = octave_shift; }

    // Scene management
    void SaveScene(int scene_index);
    void LoadScene(int scene_index);
    void ClearScene(int scene_index);

private:
    // Input Stack
    std::vector<std::unique_ptr<IInputPlugin>> input_plugins_;
    
    // Instrument
    std::unique_ptr<IInstrumentPlugin> instrument_;
    
    // Effects chain
    std::vector<std::unique_ptr<IEffectPlugin>> effects_;
    
    // Track state
    Focus focus_;
    bool muted_;
    bool soloed_;
    bool instrument_enabled_;
    char name_[32];
    
    // Track context (key, BPM, etc.)
    TrackContext context_;
    
    // Octave shift (global per track)
    OctaveShift* octave_shift_;
    
    // MIDI processing
    std::vector<MidiEvent> midi_buffer_;
    MidiEvent midi_event_buffer_[64];  // Reusable buffer for Process() to avoid stack allocation
    
    // Scene data
    struct SceneData {
        std::vector<uint8_t> input_states;
        std::vector<uint8_t> instrument_state;
        std::vector<uint8_t> effect_states;
        std::vector<MidiEvent> recorded_loops;
    };
    std::vector<SceneData> scenes_;
};

} // namespace OpenChord 