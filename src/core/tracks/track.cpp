#include "track_interface.h"
#include "../midi/octave_shift.h"
#include <cstring>

namespace OpenChord {

Track::Track() : focus_(Focus::INPUT), muted_(false), soloed_(false), instrument_enabled_(true), octave_shift_(nullptr) {
    std::strcpy(name_, "Track");
}

Track::~Track() {
}

void Track::Init() {
    // Initialize track state
    focus_ = Focus::INPUT;
    muted_ = false;
    soloed_ = false;
    instrument_enabled_ = true;
    
    // Initialize track context (key, BPM, etc.)
    context_.key = MusicalKey(0, MusicalMode::IONIAN);  // Default: C Major
    context_.bpm = 120.0f;  // Default BPM
    
    // Clear MIDI buffer
    midi_buffer_.clear();
    
    // Initialize scenes
    scenes_.resize(8);  // MAX_SCENES
}

void Track::Process(const float* const* in, float* const* out, size_t size) {
    // Skip processing if muted - clear output
    if (muted_) {
        for (size_t i = 0; i < size; i++) {
            out[0][i] = 0.0f;
            out[1][i] = 0.0f;
        }
        return;
    }
    
    // Generate MIDI from input stack (use member buffer to avoid stack allocation)
    size_t event_count = 0;
    GenerateMIDI(midi_event_buffer_, &event_count, 64);
    
    // Process MIDI through instrument (only if enabled)
    if (instrument_ && instrument_enabled_) {
        for (size_t i = 0; i < event_count; i++) {
            const MidiEvent& event = midi_event_buffer_[i];
            
            if (event.type == static_cast<uint8_t>(MidiEvent::Type::NOTE_ON)) {
                uint8_t note = event.data1;
                // Apply octave shift if available
                if (octave_shift_) {
                    note = octave_shift_->ApplyShift(note);
                }
                instrument_->NoteOn(note, event.data2 / 127.0f);
            } else if (event.type == static_cast<uint8_t>(MidiEvent::Type::NOTE_OFF)) {
                uint8_t note = event.data1;
                // Apply octave shift if available
                if (octave_shift_) {
                    note = octave_shift_->ApplyShift(note);
                }
                instrument_->NoteOff(note);
            } else if (event.type == static_cast<uint8_t>(MidiEvent::Type::PITCH_BEND)) {
                // Convert MIDI pitch bend (14-bit: 0-16383, center 8192) to semitones
                // Standard MIDI pitch bend is ±2 semitones
                int16_t pitch_bend_value = (static_cast<int16_t>(event.data2) << 7) | event.data1;
                float semitones = ((static_cast<float>(pitch_bend_value) - 8192.0f) / 8192.0f) * 2.0f;
                instrument_->SetPitchBend(semitones);
            }
        }
        
        // Process instrument audio (instruments generate from silence, so in can be nullptr)
        instrument_->Process(in, out, size);
    } else {
        // No instrument - clear output
        for (size_t i = 0; i < size; i++) {
            out[0][i] = 0.0f;
            out[1][i] = 0.0f;
        }
    }
    
    // Process effects chain
    for (auto& effect : effects_) {
        if (effect && !effect->IsBypassed()) {
            effect->Process(out, out, size);
        }
    }
}

void Track::Update() {
    // Update input plugins
    for (auto& plugin : input_plugins_) {
        if (plugin && plugin->IsActive()) {
            plugin->Update();
        }
    }
    
    // Update instrument
    if (instrument_) {
        instrument_->Update();
    }
    
    // Update effects
    for (auto& effect : effects_) {
        if (effect) {
            effect->Update();
        }
    }
}

void Track::AddInputPlugin(std::unique_ptr<IInputPlugin> plugin) {
    if (plugin) {
        input_plugins_.push_back(std::move(plugin));
    }
}

void Track::RemoveInputPlugin(size_t index) {
    if (index < input_plugins_.size()) {
        input_plugins_.erase(input_plugins_.begin() + index);
    }
}

void Track::ReorderInputPlugin(size_t from, size_t to) {
    if (from < input_plugins_.size() && to < input_plugins_.size() && from != to) {
        auto plugin = std::move(input_plugins_[from]);
        input_plugins_.erase(input_plugins_.begin() + from);
        input_plugins_.insert(input_plugins_.begin() + to, std::move(plugin));
    }
}

const std::vector<std::unique_ptr<IInputPlugin>>& Track::GetInputPlugins() const {
    return input_plugins_;
}

void Track::SetInputPluginActive(IInputPlugin* plugin, bool active) {
    if (!plugin) return;
    
    // If activating an exclusive plugin, deactivate all other exclusive plugins
    if (active && plugin->IsExclusive()) {
        for (auto& other_plugin : input_plugins_) {
            if (other_plugin.get() != plugin && 
                other_plugin->IsExclusive() && 
                other_plugin->IsActive()) {
                other_plugin->SetActive(false);
            }
        }
    }
    
    // Set the requested plugin's active state
    plugin->SetActive(active);
    
    // If turning off an input plugin, check if all exclusive INPUT plugins are now off
    // (FX and instrument plugins are separate and unaffected)
    // If so, activate Piano as the default input plugin
    if (!active) {
        bool any_exclusive_active = false;
        IInputPlugin* piano_plugin = nullptr;
        
        // Only check input plugins (FX and instruments are separate)
        for (auto& other_plugin : input_plugins_) {
            if (!other_plugin) continue;
            
            // Check if this is Notes (by name, since we can't use RTTI)
            const char* name = other_plugin->GetName();
            if (name && strcmp(name, "Notes") == 0) {
                piano_plugin = other_plugin.get();
            }
            
            // Check if any exclusive input plugin is active
            if (other_plugin->IsExclusive() && other_plugin->IsActive()) {
                any_exclusive_active = true;
            }
        }
        
        // If no exclusive input plugins are active and Piano exists, activate it as default
        if (!any_exclusive_active && piano_plugin && !piano_plugin->IsActive()) {
            piano_plugin->SetActive(true);
        }
    }
}

void Track::SetInstrument(std::unique_ptr<IInstrumentPlugin> instrument) {
    instrument_ = std::move(instrument);
}

IInstrumentPlugin* Track::GetInstrument() const {
    return instrument_.get();
}

void Track::SetInstrumentEnabled(bool enabled) {
    instrument_enabled_ = enabled;
}

bool Track::IsInstrumentEnabled() const {
    return instrument_enabled_;
}

void Track::AddEffect(std::unique_ptr<IEffectPlugin> effect) {
    if (effect) {
        effects_.push_back(std::move(effect));
    }
}

void Track::RemoveEffect(size_t index) {
    if (index < effects_.size()) {
        effects_.erase(effects_.begin() + index);
    }
}

void Track::ReorderEffect(size_t from, size_t to) {
    if (from < effects_.size() && to < effects_.size() && from != to) {
        auto effect = std::move(effects_[from]);
        effects_.erase(effects_.begin() + from);
        effects_.insert(effects_.begin() + to, std::move(effect));
    }
}

const std::vector<std::unique_ptr<IEffectPlugin>>& Track::GetEffects() const {
    return effects_;
}

void Track::SetFocus(Focus focus) {
    focus_ = focus;
}

Track::Focus Track::GetFocus() const {
    return focus_;
}

void Track::UpdateUI() {
    // Update UI based on focus
    switch (focus_) {
        case Focus::INPUT:
            for (auto& plugin : input_plugins_) {
                if (plugin) plugin->UpdateUI();
            }
            break;
        case Focus::INSTRUMENT:
            if (instrument_) instrument_->UpdateUI();
            break;
        case Focus::FX:
            for (auto& effect : effects_) {
                if (effect) effect->UpdateUI();
            }
            break;
    }
}

void Track::HandleEncoder(int encoder, float delta) {
    // Route to focused component
    switch (focus_) {
        case Focus::INPUT:
            if (!input_plugins_.empty()) {
                input_plugins_.back()->HandleEncoder(encoder, delta);
            }
            break;
        case Focus::INSTRUMENT:
            if (instrument_) {
                instrument_->HandleEncoder(encoder, delta);
            }
            break;
        case Focus::FX:
            if (!effects_.empty()) {
                effects_.back()->HandleEncoder(encoder, delta);
            }
            break;
    }
}

void Track::HandleButton(int button, bool pressed) {
    // Route to focused component
    switch (focus_) {
        case Focus::INPUT:
            for (auto& plugin : input_plugins_) {
                if (plugin) plugin->HandleButton(button, pressed);
            }
            break;
        case Focus::INSTRUMENT:
            if (instrument_) {
                instrument_->HandleButton(button, pressed);
            }
            break;
        case Focus::FX:
            for (auto& effect : effects_) {
                if (effect) effect->HandleButton(button, pressed);
            }
            break;
    }
}

void Track::HandleJoystick(float x, float y) {
    // Route to focused component
    switch (focus_) {
        case Focus::INPUT:
            for (auto& plugin : input_plugins_) {
                if (plugin) plugin->HandleJoystick(x, y);
            }
            break;
        case Focus::INSTRUMENT:
            if (instrument_) {
                instrument_->HandleJoystick(x, y);
            }
            break;
        case Focus::FX:
            for (auto& effect : effects_) {
                if (effect) effect->HandleJoystick(x, y);
            }
            break;
    }
}

void Track::ProcessMIDI(const MidiEvent* events, size_t count) {
    // Process MIDI through input plugins
    for (auto& plugin : input_plugins_) {
        if (plugin && plugin->IsActive()) {
            plugin->ProcessMIDI(events, count);
        }
    }
}

void Track::GenerateMIDI(MidiEvent* events, size_t* count, size_t max_events) {
    *count = 0;
    
    // Generate MIDI from input plugins
    // Priority: Check BasicMidiInput (external MIDI) first if present, then other plugins
    // This allows external MIDI to play the instrument while built-in keys can still generate MIDI for output
    // Process plugins in order, but prioritize BasicMidiInput (priority 100)
    
    // First pass: Look for BasicMidiInput (external MIDI passthrough)
    IInputPlugin* basic_midi_plugin = nullptr;
    for (auto& plugin : input_plugins_) {
        if (plugin && plugin->IsActive()) {
            const char* name = plugin->GetName();
            if (name && strcmp(name, "MIDI Input") == 0) {
                basic_midi_plugin = plugin.get();
                break;
            }
        }
    }
    
    // If BasicMidiInput is active, check it first for external MIDI
    if (basic_midi_plugin && basic_midi_plugin->IsActive()) {
        size_t plugin_count = 0;
        basic_midi_plugin->GenerateMIDI(events + *count, &plugin_count, max_events - *count);
        
        if (plugin_count > 0) {
            // External MIDI found - use it for instrument playback
            *count += plugin_count;
            return;
        }
    }
    
    // Second pass: Check other plugins (built-in keys, etc.)
    // Process plugins in order, but stop after first plugin generates MIDI
    // This ensures only one input mode (chord mapping OR chromatic) generates MIDI at a time
    for (auto& plugin : input_plugins_) {
        if (plugin && plugin->IsActive()) {
            // Skip BasicMidiInput - already checked above
            const char* name = plugin->GetName();
            if (name && strcmp(name, "MIDI Input") == 0) {
                continue;
            }
            
            size_t plugin_count = 0;
            plugin->GenerateMIDI(events + *count, &plugin_count, max_events - *count);
            
            if (plugin_count > 0) {
                // This plugin generated MIDI, stop processing other plugins
                *count += plugin_count;
                break;
            }
            
            if (*count >= max_events) break;
        }
    }
}

void Track::SetMute(bool mute) {
    muted_ = mute;
}

bool Track::IsMuted() const {
    return muted_;
}

void Track::SetSolo(bool solo) {
    soloed_ = solo;
}

bool Track::IsSoloed() const {
    return soloed_;
}

void Track::SetName(const char* name) {
    if (name) {
        std::strncpy(name_, name, sizeof(name_) - 1);
        name_[sizeof(name_) - 1] = '\0';
    }
}

const char* Track::GetName() const {
    return name_;
}

void Track::SetKey(MusicalKey key) {
    context_.key = key;
    
    // Notify all input plugins that key changed
    // Plugins can query the key via GetKey() or GetContext()
    for (auto& plugin : input_plugins_) {
        if (plugin) {
            // Plugins will read the key when they need it
            // No explicit notification needed - they can poll GetKey()
        }
    }
}

MusicalKey Track::GetKey() const {
    return context_.key;
}

void Track::SaveScene(int scene_index) {
    if (scene_index < 0 || scene_index >= scenes_.size()) return;
    
    // TODO: Implement scene saving
    // This would save the current state of all plugins and recorded loops
}

void Track::LoadScene(int scene_index) {
    if (scene_index < 0 || scene_index >= scenes_.size()) return;
    
    // TODO: Implement scene loading
    // This would restore the state of all plugins and recorded loops
}

void Track::ClearScene(int scene_index) {
    if (scene_index < 0 || scene_index >= scenes_.size()) return;
    
    // TODO: Implement scene clearing
    // This would clear the saved state for the scene
}

} // namespace OpenChord 