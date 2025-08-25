#pragma once

#include <vector>
#include <cstdint>
#include "daisy_seed.h"
#include "hid/midi.h"

namespace OpenChord {

// MIDI event structure using Daisy's types directly
struct MidiEvent {
    enum class Source {
        USB,        // USB MIDI input
        TRS_IN,     // TRS MIDI input
        TRS_OUT,    // TRS MIDI output
        GENERATED,  // Built-in controls
        INTERNAL    // Internal processing
    };
    
    daisy::MidiMessageType type;  // Use Daisy's type directly
    uint8_t channel;
    uint8_t data[2];
    uint32_t timestamp; // System timestamp when event was received
    Source source;       // Where this MIDI event came from
    
    MidiEvent() : type(daisy::MidiMessageType::NoteOff), channel(0), data{0, 0}, timestamp(0), source(Source::INTERNAL) {}
    MidiEvent(daisy::MidiMessageType t, uint8_t ch, uint8_t d0, uint8_t d1, Source s = Source::INTERNAL, uint32_t ts = 0) 
        : type(t), channel(ch), data{d0, d1}, source(s), timestamp(ts) {}
};

// Centralized MIDI data hub - accessible to all classes
class MidiHub {
private:
    static MidiHub* instance_;
    
    // Input MIDI events (from external sources)
    std::vector<MidiEvent> usb_input_events_;
    std::vector<MidiEvent> trs_input_events_;
    
    // Generated MIDI events (from built-in keys, etc.)
    std::vector<MidiEvent> generated_events_;
    
    // Combined MIDI events (all inputs + generated)
    std::vector<MidiEvent> combined_events_;
    
    // MIDI clock and timing data
    uint32_t midi_clock_;
    uint32_t last_clock_timestamp_;
    float bpm_;
    
    // MIDI routing and filtering
    bool usb_input_enabled_;
    bool trs_input_enabled_;
    bool trs_output_enabled_;
    bool generated_enabled_;
    
    // TRS MIDI output buffer
    std::vector<MidiEvent> trs_output_buffer_;
    
public:
    MidiHub();
    ~MidiHub();
    
    // Singleton access
    static MidiHub* GetInstance();
    
    // USB MIDI handling
    void AddUsbInputEvent(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1);
    void AddUsbInputEvent(const MidiEvent& event);
    void ClearUsbInputEvents();
    const std::vector<MidiEvent>& GetUsbInputEvents() const { return usb_input_events_; }
    
    // TRS MIDI input handling
    void AddTrsInputEvent(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1);
    void AddTrsInputEvent(const MidiEvent& event);
    void ClearTrsInputEvents();
    const std::vector<MidiEvent>& GetTrsInputEvents() const { return trs_input_events_; }
    
    // TRS MIDI output handling
    void AddTrsOutputEvent(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1);
    void AddTrsOutputEvent(const MidiEvent& event);
    void ClearTrsOutputBuffer();
    const std::vector<MidiEvent>& GetTrsOutputBuffer() const { return trs_output_buffer_; }
    
    // Generated MIDI handling
    void AddGeneratedEvent(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1);
    void AddGeneratedEvent(const MidiEvent& event);
    void ClearGeneratedEvents();
    const std::vector<MidiEvent>& GetGeneratedEvents() const { return generated_events_; }
    
    // Combined MIDI access
    void UpdateCombinedEvents();
    const std::vector<MidiEvent>& GetCombinedEvents() const { return combined_events_; }
    
    // MIDI timing
    void SetMidiClock(uint32_t clock);
    uint32_t GetMidiClock() const { return midi_clock_; }
    void SetBPM(float bpm);
    float GetBPM() const { return bpm_; }
    
    // MIDI routing control
    void EnableUsbInput(bool enable) { usb_input_enabled_ = enable; }
    void EnableTrsInput(bool enable) { trs_input_enabled_ = enable; }
    void EnableTrsOutput(bool enable) { trs_output_enabled_ = enable; }
    void EnableGenerated(bool enable) { generated_enabled_ = enable; }
    
    bool IsUsbInputEnabled() const { return usb_input_enabled_; }
    bool IsTrsInputEnabled() const { return trs_input_enabled_; }
    bool IsTrsOutputEnabled() const { return trs_output_enabled_; }
    bool IsGeneratedEnabled() const { return generated_enabled_; }
    
    // Utility functions
    size_t GetUsbInputEventCount() const { return usb_input_events_.size(); }
    size_t GetTrsInputEventCount() const { return trs_input_events_.size(); }
    size_t GetTrsOutputEventCount() const { return trs_output_buffer_.size(); }
    size_t GetGeneratedEventCount() const { return generated_events_.size(); }
    size_t GetCombinedEventCount() const { return combined_events_.size(); }
    
    // Clear all events
    void ClearAllEvents();
};

// Convenience namespace for easy MidiHub access
namespace Midi {
    // USB MIDI input
    inline void AddUsbInputEvent(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1) {
        if (MidiHub* hub = MidiHub::GetInstance()) {
            hub->AddUsbInputEvent(type, channel, data0, data1);
        }
    }
    
    inline void AddUsbInputEvent(const MidiEvent& event) {
        if (MidiHub* hub = MidiHub::GetInstance()) {
            hub->AddUsbInputEvent(event);
        }
    }
    
    // TRS MIDI input
    inline void AddTrsInputEvent(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1) {
        if (MidiHub* hub = MidiHub::GetInstance()) {
            hub->AddTrsInputEvent(type, channel, data0, data1);
        }
    }
    
    inline void AddTrsInputEvent(const MidiEvent& event) {
        if (MidiHub* hub = MidiHub::GetInstance()) {
            hub->AddTrsInputEvent(event);
        }
    }
    
    // TRS MIDI output
    inline void AddTrsOutputEvent(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1) {
        if (MidiHub* hub = MidiHub::GetInstance()) {
            hub->AddTrsOutputEvent(type, channel, data0, data1);
        }
    }
    
    inline void AddTrsOutputEvent(const MidiEvent& event) {
        if (MidiHub* hub = MidiHub::GetInstance()) {
            hub->AddTrsOutputEvent(event);
        }
    }
    
    // Generated MIDI
    inline void AddGeneratedEvent(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1) {
        if (MidiHub* hub = MidiHub::GetInstance()) {
            hub->AddGeneratedEvent(type, channel, data0, data1);
        }
    }
    
    inline void AddGeneratedEvent(const MidiEvent& event) {
        if (MidiHub* hub = MidiHub::GetInstance()) {
            hub->AddGeneratedEvent(event);
        }
    }
    
    // Get MIDI events by source
    const std::vector<MidiEvent>& GetUsbInputEvents();
    const std::vector<MidiEvent>& GetTrsInputEvents();
    const std::vector<MidiEvent>& GetTrsOutputBuffer();
    const std::vector<MidiEvent>& GetGeneratedEvents();
    const std::vector<MidiEvent>& GetCombinedEvents();
    
    // MIDI timing
    uint32_t GetMidiClock();
    float GetBPM();
    
    // MIDI routing
    void EnableUsbInput(bool enable);
    void EnableTrsInput(bool enable);
    void EnableTrsOutput(bool enable);
    void EnableGenerated(bool enable);
    
    // Clear events
    void ClearUsbInputEvents();
    void ClearTrsInputEvents();
    void ClearTrsOutputBuffer();
    void ClearGeneratedEvents();
    void ClearAllEvents();
}

} // namespace OpenChord
