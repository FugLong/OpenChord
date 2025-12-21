#include "midi_handler.h"
#include "daisy_seed.h"

// Debug mode - disable USB MIDI to allow serial output for debugging
#define DEBUG_MODE false

namespace OpenChord {

OpenChordMidiHandler::OpenChordMidiHandler() 
    : usb_midi_initialized_(false), trs_midi_initialized_(false), hw_(nullptr) {
}

OpenChordMidiHandler::~OpenChordMidiHandler() {
}

void OpenChordMidiHandler::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    
    // Initialize USB MIDI only when not in debug mode
    #if DEBUG_MODE == false
    daisy::MidiUsbHandler::Config usb_config;
    // Use EXTERNAL for external USB pins (D29/D30 = pins 36-37)
    // INTERNAL = micro USB port, EXTERNAL = external USB pins (D29/D30 = pins 36-37)
    usb_config.transport_config.periph = daisy::MidiUsbTransport::Config::EXTERNAL;
    usb_config.transport_config.tx_retry_count = 3;
    
    usb_midi_.Init(usb_config);
    usb_midi_.StartReceive();
    usb_midi_initialized_ = true;
    #else
    usb_midi_initialized_ = false;
    #endif
    
    // Initialize TRS MIDI using official Daisy Seed example pattern
    daisy::MidiUartHandler::Config trs_config;
    trs_config.transport_config.periph = daisy::UartHandler::Config::Peripheral::UART_4;
    trs_config.transport_config.rx = daisy::Pin(daisy::PORTB, 8);  // PB8 = Pin 12
    trs_config.transport_config.tx = daisy::Pin(daisy::PORTB, 9);  // PB9 = Pin 13
    
    trs_midi_.Init(trs_config);
    trs_midi_.StartReceive();
    trs_midi_initialized_ = true;
}

void OpenChordMidiHandler::ProcessMidi(AudioEngine* audio_engine) {
    // Process MIDI from enabled sources and add to MidiHub
    if (usb_midi_initialized_) {
        ProcessUsbMidi();
    }
    if (trs_midi_initialized_) {
        ProcessTrsMidi();
    }
}

void OpenChordMidiHandler::ProcessUsbMidi() {
    if (!usb_midi_initialized_) return;
    
    // Keep MIDI listening active
    usb_midi_.Listen();
    
    // Process any incoming MIDI events
    while (usb_midi_.HasEvents()) {
        daisy::MidiEvent event = usb_midi_.PopEvent();
        AddToMidiHub(event, MidiEvent::Source::USB);
    }
}

void OpenChordMidiHandler::ProcessTrsMidi() {
    if (!trs_midi_initialized_) return;
    
    // Process MIDI in the background (official Daisy Seed example pattern)
    trs_midi_.Listen();
    
    // Loop through any MIDI Events
    while (trs_midi_.HasEvents()) {
        daisy::MidiEvent event = trs_midi_.PopEvent();
        AddToMidiHub(event, MidiEvent::Source::TRS_IN);
    }
}


void OpenChordMidiHandler::SendMidi(const MidiEvent& event) {
    // Send to USB MIDI
    if (usb_midi_initialized_) {
        // Convert OpenChord MidiEvent to raw MIDI bytes and send
        uint8_t midi_bytes[3];
        size_t byte_count = 0;
        ConvertToMidiBytes(event, midi_bytes, &byte_count);
        
        if (byte_count > 0) {
            usb_midi_.SendMessage(midi_bytes, byte_count);
        }
    }
    
    // Send to TRS MIDI
    if (trs_midi_initialized_) {
        uint8_t midi_bytes[3];
        size_t byte_count = 0;
        ConvertToMidiBytes(event, midi_bytes, &byte_count);
        
        if (byte_count > 0) {
            trs_midi_.SendMessage(midi_bytes, byte_count);
        }
    }
}

void OpenChordMidiHandler::SendMidi(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1) {
    MidiEvent event(type, channel, data0, data1);
    SendMidi(event);
}

void OpenChordMidiHandler::AddToMidiHub(const daisy::MidiEvent& event, MidiEvent::Source source) {
    // Convert Daisy MidiEvent to OpenChord MidiEvent and add to hub
    MidiEvent openchord_event;
    openchord_event.type = event.type;
    openchord_event.channel = event.channel;
    openchord_event.data[0] = event.data[0];
    openchord_event.data[1] = event.data[1];
    openchord_event.source = source;
    openchord_event.timestamp = hw_->system.GetNow();
    
    // Add to global MIDI hub based on source
    switch (source) {
        case MidiEvent::Source::USB:
            OpenChord::Midi::AddUsbInputEvent(openchord_event);
            break;
        case MidiEvent::Source::TRS_IN:
            OpenChord::Midi::AddTrsInputEvent(openchord_event);
            break;
        default:
            break;
    }
}

void OpenChordMidiHandler::ConvertToMidiBytes(const MidiEvent& event, uint8_t* bytes, size_t* size) {
    *size = 0;
    
    // Convert OpenChord MidiEvent to raw MIDI bytes
    switch (event.type) {
        case daisy::MidiMessageType::NoteOn:
            bytes[0] = 0x90 | (event.channel & 0x0F);
            bytes[1] = event.data[0];
            bytes[2] = event.data[1];
            *size = 3;
            break;
            
        case daisy::MidiMessageType::NoteOff:
            bytes[0] = 0x80 | (event.channel & 0x0F);
            bytes[1] = event.data[0];
            bytes[2] = event.data[1];
            *size = 3;
            break;
            
        case daisy::MidiMessageType::ControlChange:
            bytes[0] = 0xB0 | (event.channel & 0x0F);
            bytes[1] = event.data[0];
            bytes[2] = event.data[1];
            *size = 3;
            break;
            
        default:
            // Unsupported message type
            break;
    }
}

} // namespace OpenChord
