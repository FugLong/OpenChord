#include "midi_router.h"
#include "midi/midi_interface.h"
#include "tracks/track_interface.h"
#include <cstring>

namespace OpenChord {

MidiRouter::MidiRouter()
    : system_(nullptr)
    , midi_handler_(nullptr)
    , octave_shift_(nullptr)
{
}

MidiRouter::~MidiRouter() {
}

void MidiRouter::Init(OpenChordSystem* system, OpenChordMidiHandler* midi_handler,
                     OctaveShift* octave_shift) {
    system_ = system;
    midi_handler_ = midi_handler;
    octave_shift_ = octave_shift;
}

void MidiRouter::RouteMIDI() {
    if (!system_ || !midi_handler_) return;
    
    // Process incoming MIDI events from hardware
    midi_handler_->ProcessMidi();
    
    // Route external MIDI input to tracks
    RouteExternalMIDI();
    
    // Route track-generated MIDI to outputs
    RouteGeneratedMIDI();
}

void MidiRouter::RouteExternalMIDI() {
    if (!system_) return;
    
    // Get USB and TRS input events separately (NOT combined - we don't want generated events here)
    const std::vector<MidiHubEvent>& usb_input_events = Midi::GetUsbInputEvents();
    const std::vector<MidiHubEvent>& trs_input_events = Midi::GetTrsInputEvents();
    
    // Convert MidiHubEvent to track MidiEvent and route to active track
    size_t track_event_count = 0;
    const size_t max_events = MAX_EVENTS;
    
    // Process USB input events
    for (const auto& hub_event : usb_input_events) {
        if (track_event_count >= max_events) break;
        MidiEvent track_event = ConvertHubToTrackEvent(hub_event);
        if (track_event.type == 0) continue;  // Skip unsupported types
        track_events_[track_event_count++] = track_event;
    }
    
    // Process TRS input events
    for (const auto& hub_event : trs_input_events) {
        if (track_event_count >= max_events) break;
        MidiEvent track_event = ConvertHubToTrackEvent(hub_event);
        if (track_event.type == 0) continue;  // Skip unsupported types
        track_events_[track_event_count++] = track_event;
    }
    
    // Route MIDI events to active track via system
    if (track_event_count > 0) {
        system_->ProcessMIDI(track_events_, track_event_count);
    }
    
    // Clear processed input events from hub to prevent reprocessing
    if (!usb_input_events.empty()) {
        Midi::ClearUsbInputEvents();
    }
    if (!trs_input_events.empty()) {
        Midi::ClearTrsInputEvents();
    }
}

void MidiRouter::RouteGeneratedMIDI() {
    if (!system_ || !midi_handler_) return;
    
    // Generate MIDI from active track's input stack and route through MIDI hub
    // Generate MIDI from built-in controls (keys, joystick, etc.) - skip BasicMidiInput to avoid echo
    Track* active_track = system_->GetTrack(system_->GetActiveTrack());
    if (!active_track) return;
    
    size_t midi_event_count = 0;
    
    // Generate MIDI from input plugins, but skip BasicMidiInput (external MIDI input)
    const auto& plugins = active_track->GetInputPlugins();
    for (const auto& plugin : plugins) {
        if (!plugin || !plugin->IsActive()) continue;
        
        // Skip BasicMidiInput - we don't want to echo external MIDI back out
        const char* name = plugin->GetName();
        if (IsBasicMidiInputPlugin(name)) {
            continue;
        }
        
        // Generate MIDI from this plugin
        size_t plugin_count = 0;
        plugin->GenerateMIDI(generated_events_ + midi_event_count, &plugin_count, MAX_EVENTS - midi_event_count);
        
        if (plugin_count > 0) {
            // This plugin generated MIDI, stop processing other plugins
            midi_event_count += plugin_count;
            break;
        }
        
        if (midi_event_count >= MAX_EVENTS) break;
    }
    
    // Add generated MIDI events to hub (for routing to USB/TRS outputs)
    // Apply octave shift and convert to hub events
    for (size_t i = 0; i < midi_event_count; i++) {
        MidiEvent event = generated_events_[i];
        
        // Apply octave shift to note messages
        if (octave_shift_ && 
            (event.type == static_cast<uint8_t>(MidiEvent::Type::NOTE_ON) ||
             event.type == static_cast<uint8_t>(MidiEvent::Type::NOTE_OFF))) {
            event.data1 = octave_shift_->ApplyShift(event.data1);
        }
        
        // Convert MidiEvent::Type to daisy::MidiMessageType
        daisy::MidiMessageType msg_type = ConvertTrackEventToHubType(event);
        
        // Add to MIDI hub as GENERATED event
        Midi::AddGeneratedEvent(msg_type, event.channel, event.data1, event.data2);
    }
    
    // Send generated MIDI events from hub to USB and TRS outputs
    const std::vector<MidiHubEvent>& generated_events = Midi::GetGeneratedEvents();
    for (const auto& hub_event : generated_events) {
        midi_handler_->SendMidi(hub_event);
    }
    
    // Clear generated events after sending
    if (!generated_events.empty()) {
        Midi::ClearGeneratedEvents();
    }
}

MidiEvent MidiRouter::ConvertHubToTrackEvent(const MidiHubEvent& hub_event) {
    MidiEvent track_event;
    track_event.channel = hub_event.channel;
    track_event.data1 = hub_event.data[0];
    track_event.data2 = hub_event.data[1];
    track_event.timestamp = hub_event.timestamp;
    
    // Convert daisy::MidiMessageType to track MidiEvent::Type
    switch (hub_event.type) {
        case daisy::MidiMessageType::NoteOn:
            track_event.type = static_cast<uint8_t>(MidiEvent::Type::NOTE_ON);
            break;
        case daisy::MidiMessageType::NoteOff:
            track_event.type = static_cast<uint8_t>(MidiEvent::Type::NOTE_OFF);
            break;
        case daisy::MidiMessageType::ControlChange:
            track_event.type = static_cast<uint8_t>(MidiEvent::Type::CONTROL_CHANGE);
            break;
        case daisy::MidiMessageType::PitchBend:
            track_event.type = static_cast<uint8_t>(MidiEvent::Type::PITCH_BEND);
            break;
        default:
            track_event.type = 0;  // Invalid - caller should skip
            break;
    }
    return track_event;
}

daisy::MidiMessageType MidiRouter::ConvertTrackEventToHubType(const MidiEvent& track_event) {
    if (track_event.type == static_cast<uint8_t>(MidiEvent::Type::NOTE_ON)) {
        return daisy::MidiMessageType::NoteOn;
    } else if (track_event.type == static_cast<uint8_t>(MidiEvent::Type::NOTE_OFF)) {
        return daisy::MidiMessageType::NoteOff;
    } else if (track_event.type == static_cast<uint8_t>(MidiEvent::Type::PITCH_BEND)) {
        return daisy::MidiMessageType::PitchBend;
    } else if (track_event.type == static_cast<uint8_t>(MidiEvent::Type::CONTROL_CHANGE)) {
        return daisy::MidiMessageType::ControlChange;
    }
    // Return NoteOff as default (safer than invalid type)
    return daisy::MidiMessageType::NoteOff;
}

bool MidiRouter::IsBasicMidiInputPlugin(const char* plugin_name) const {
    if (!plugin_name) return false;
    return (strcmp(plugin_name, "MIDI Input") == 0);
}

} // namespace OpenChord

