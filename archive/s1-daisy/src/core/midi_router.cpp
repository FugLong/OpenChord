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
    
    // Process incoming MIDI events from hardware (needed for RouteExternalMIDI)
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
    
    // Read generated MIDI events from hub (consuming read)
    // Track::GenerateMIDI() adds events to hub when it reads from plugins
    // Audio engine reads from plugin buffers directly, so consuming from hub is safe
    std::vector<MidiHubEvent> hub_events;
    Midi::ConsumeGeneratedEvents(hub_events);
    
    // Convert hub events to track events, filter out BasicMidiInput (external MIDI echo), and send
    for (const auto& hub_event : hub_events) {
        // Skip external MIDI input events (we don't want to echo them back)
        // Generated events from built-in controls have source GENERATED
        if (hub_event.source != MidiHubEvent::Source::GENERATED) {
            continue;
        }
        
        // Convert hub event to track event format
        MidiEvent track_event = ConvertHubToTrackEvent(hub_event);
        if (track_event.type == 0) continue;  // Skip unsupported types
        
        // Apply octave shift to note messages
        if (octave_shift_ && 
            (track_event.type == static_cast<uint8_t>(MidiEvent::Type::NOTE_ON) ||
             track_event.type == static_cast<uint8_t>(MidiEvent::Type::NOTE_OFF))) {
            track_event.data1 = octave_shift_->ApplyShift(track_event.data1);
        }
        
        // Convert back to hub format and send
        daisy::MidiMessageType msg_type = ConvertTrackEventToHubType(track_event);
        midi_handler_->SendMidi(msg_type, track_event.channel, track_event.data1, track_event.data2);
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
    // Convert track MidiEvent::Type (which uses MIDI status byte values) to daisy::MidiMessageType
    // MidiEvent::Type uses raw MIDI status bytes: NOTE_ON=0x90, NOTE_OFF=0x80, etc.
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
    // This should not happen for valid note events, but provides a fallback
    return daisy::MidiMessageType::NoteOff;
}

bool MidiRouter::IsBasicMidiInputPlugin(const char* plugin_name) const {
    if (!plugin_name) return false;
    return (strcmp(plugin_name, "MIDI Input") == 0);
}

} // namespace OpenChord

