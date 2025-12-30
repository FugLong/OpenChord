#include "midi_interface.h"
#include <algorithm>

namespace OpenChord {

// Initialize static instance
MidiHub* MidiHub::instance_ = nullptr;

MidiHub::MidiHub() 
    : midi_clock_(0)
    , last_clock_timestamp_(0)
    , bpm_(120.0f)
    , usb_input_enabled_(true)
    , trs_input_enabled_(true)
    , trs_output_enabled_(true)
    , generated_enabled_(true) {
}

MidiHub::~MidiHub() {
    instance_ = nullptr;
}

MidiHub* MidiHub::GetInstance() {
    if (!instance_) {
        instance_ = new MidiHub();
    }
    return instance_;
}

void MidiHub::AddUsbInputEvent(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1) {
    MidiHubEvent event(type, channel, data0, data1, MidiHubEvent::Source::USB);
    AddUsbInputEvent(event);
}

void MidiHub::AddTrsInputEvent(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1) {
    MidiHubEvent event(type, channel, data0, data1, MidiHubEvent::Source::TRS_IN);
    AddTrsInputEvent(event);
}

void MidiHub::AddTrsOutputEvent(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1) {
    MidiHubEvent event(type, channel, data0, data1, MidiHubEvent::Source::TRS_OUT);
    AddTrsOutputEvent(event);
}

void MidiHub::AddGeneratedEvent(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1) {
    MidiHubEvent event(type, channel, data0, data1, MidiHubEvent::Source::GENERATED);
    AddGeneratedEvent(event);
}

void MidiHub::AddUsbInputEvent(const MidiHubEvent& event) {
    if (!usb_input_enabled_) return;
    
    usb_input_events_.push_back(event);
    
    // Keep buffer size reasonable (last 1000 events)
    if (usb_input_events_.size() > 1000) {
        usb_input_events_.erase(usb_input_events_.begin());
    }
}

void MidiHub::ClearUsbInputEvents() {
    usb_input_events_.clear();
}

void MidiHub::AddTrsInputEvent(const MidiHubEvent& event) {
    if (!trs_input_enabled_) return;
    
    trs_input_events_.push_back(event);
    
    // Keep buffer size reasonable (last 1000 events)
    if (trs_input_events_.size() > 1000) {
        trs_input_events_.erase(trs_input_events_.begin());
    }
}

void MidiHub::ClearTrsInputEvents() {
    trs_input_events_.clear();
}

void MidiHub::AddTrsOutputEvent(const MidiHubEvent& event) {
    if (!trs_output_enabled_) return;
    
    trs_output_buffer_.push_back(event);
    
    // Keep buffer size reasonable (last 1000 events)
    if (trs_output_buffer_.size() > 1000) {
        trs_output_buffer_.erase(trs_output_buffer_.begin());
    }
}

void MidiHub::ClearTrsOutputBuffer() {
    trs_output_buffer_.clear();
}

void MidiHub::AddGeneratedEvent(const MidiHubEvent& event) {
    if (!generated_enabled_) return;
    
    generated_events_.push_back(event);
    
    // Keep buffer size reasonable (last 1000 events)
    if (generated_events_.size() > 1000) {
        generated_events_.erase(generated_events_.begin());
    }
}

void MidiHub::ClearGeneratedEvents() {
    generated_events_.clear();
}

void MidiHub::UpdateCombinedEvents() {
    combined_events_.clear();
    
    // Add USB input events first (external USB MIDI has priority)
    combined_events_.insert(combined_events_.end(), usb_input_events_.begin(), usb_input_events_.end());
    
    // Add TRS input events
    combined_events_.insert(combined_events_.end(), trs_input_events_.begin(), trs_input_events_.end());
    
    // Add generated events
    combined_events_.insert(combined_events_.end(), generated_events_.begin(), generated_events_.end());
    
    // Sort by timestamp if needed
    std::sort(combined_events_.begin(), combined_events_.end(), 
              [](const MidiHubEvent& a, const MidiHubEvent& b) {
                  return a.timestamp < b.timestamp;
              });
}

void MidiHub::SetMidiClock(uint32_t clock) {
    midi_clock_ = clock;
    last_clock_timestamp_ = clock;
}

void MidiHub::SetBPM(float bpm) {
    bpm_ = bpm;
}

void MidiHub::ClearAllEvents() {
    usb_input_events_.clear();
    trs_input_events_.clear();
    trs_output_buffer_.clear();
    generated_events_.clear();
    combined_events_.clear();
}

// Midi namespace function implementations
namespace Midi {

// Shared static empty vector to avoid dangling references
static const std::vector<MidiHubEvent> kEmptyMidiEvents;

const std::vector<MidiHubEvent>& GetUsbInputEvents() {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) {
        return hub->GetUsbInputEvents();
    }
    return kEmptyMidiEvents;
}

const std::vector<MidiHubEvent>& GetTrsInputEvents() {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) {
        return hub->GetTrsInputEvents();
    }
    return kEmptyMidiEvents;
}

const std::vector<MidiHubEvent>& GetTrsOutputBuffer() {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) {
        return hub->GetTrsOutputBuffer();
    }
    return kEmptyMidiEvents;
}

const std::vector<MidiHubEvent>& GetGeneratedEvents() {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) {
        return hub->GetGeneratedEvents();
    }
    return kEmptyMidiEvents;
}

const std::vector<MidiHubEvent>& GetCombinedEvents() {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) {
        hub->UpdateCombinedEvents();
        return hub->GetCombinedEvents();
    }
    return kEmptyMidiEvents;
}

uint32_t GetMidiClock() {
    MidiHub* hub = MidiHub::GetInstance();
    return hub ? hub->GetMidiClock() : 0;
}

float GetBPM() {
    MidiHub* hub = MidiHub::GetInstance();
    return hub ? hub->GetBPM() : 120.0f;
}

void EnableUsbInput(bool enable) {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) hub->EnableUsbInput(enable);
}

void EnableTrsInput(bool enable) {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) hub->EnableTrsInput(enable);
}

void EnableTrsOutput(bool enable) {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) hub->EnableTrsOutput(enable);
}

void EnableGenerated(bool enable) {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) hub->EnableGenerated(enable);
}

void ClearUsbInputEvents() {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) hub->ClearUsbInputEvents();
}

void ClearTrsInputEvents() {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) hub->ClearTrsInputEvents();
}

void ClearTrsOutputBuffer() {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) hub->ClearTrsOutputBuffer();
}

void ClearGeneratedEvents() {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) hub->ClearGeneratedEvents();
}

void ClearAllEvents() {
    MidiHub* hub = MidiHub::GetInstance();
    if (hub) hub->ClearAllEvents();
}

} // namespace Midi
} // namespace OpenChord
