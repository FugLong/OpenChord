#include "midi_handler.h"
#include <cmath>

namespace OpenChord {

OpenChordMidiHandler::OpenChordMidiHandler() 
    : usb_midi_initialized_(false), trs_midi_initialized_(false), hw_(nullptr) {
}

OpenChordMidiHandler::~OpenChordMidiHandler() {
}

void OpenChordMidiHandler::Init(daisy::DaisySeed* hw) {
    hw_ = hw;
    
    // Initialize USB MIDI
    daisy::MidiUsbHandler::Config usb_config;
    usb_config.transport_config.periph = daisy::MidiUsbTransport::Config::INTERNAL;
    usb_config.transport_config.tx_retry_count = 3;
    
    usb_midi_.Init(usb_config);
    usb_midi_.StartReceive();
    usb_midi_initialized_ = true;
    
    // Initialize TRS MIDI (UART4 on pins D11/D12)
    daisy::MidiUartHandler::Config trs_config;
    trs_config.transport_config.rx = daisy::seed::D11;  // Pin 12
    trs_config.transport_config.tx = daisy::seed::D12;  // Pin 13
    trs_config.transport_config.periph = daisy::UartHandler::Config::Peripheral::UART_4;
    
    trs_midi_.Init(trs_config);
    trs_midi_.StartReceive();
    trs_midi_initialized_ = true;
}

void OpenChordMidiHandler::ProcessMidi(AudioEngine* audio_engine) {
    if (audio_engine) {
        ProcessUsbMidi(audio_engine);
        ProcessTrsMidi(audio_engine);
    }
}

void OpenChordMidiHandler::ProcessUsbMidi(AudioEngine* audio_engine) {
    if (!usb_midi_initialized_) return;
    
    // Keep MIDI listening active
    usb_midi_.Listen();
    
    // Process any incoming MIDI events
    while (usb_midi_.HasEvents()) {
        daisy::MidiEvent event = usb_midi_.PopEvent();
        ProcessMidiEvent(event, audio_engine);
        AddToMidiHub(event, MidiEvent::Source::USB);
    }
}

void OpenChordMidiHandler::ProcessTrsMidi(AudioEngine* audio_engine) {
    if (!trs_midi_initialized_) return;
    
    // Keep MIDI listening active
    trs_midi_.Listen();
    
    // Process any incoming MIDI events
    while (trs_midi_.HasEvents()) {
        daisy::MidiEvent event = trs_midi_.PopEvent();
        ProcessMidiEvent(event, audio_engine);
        AddToMidiHub(event, MidiEvent::Source::TRS_IN);
    }
}

void OpenChordMidiHandler::ProcessMidiEvent(const daisy::MidiEvent& event, AudioEngine* audio_engine) {
    switch (event.type) {
        case daisy::MidiMessageType::NoteOn:
            if (event.data[1] > 0) { // Velocity > 0
                float freq = mtof(event.data[0]);
                audio_engine->SetFrequency(freq);
                audio_engine->NoteOn();
            } else {
                audio_engine->NoteOff();
            }
            break;
            
        case daisy::MidiMessageType::NoteOff:
            audio_engine->NoteOff();
            break;
            
        case daisy::MidiMessageType::ControlChange:
            // Handle common MIDI CC messages
            switch (event.data[0]) {
                case 1: // Modulation wheel
                    // Could be used for vibrato or other modulation
                    break;
                case 7: // Volume
                    // Could be used to override volume pot
                    break;
                case 10: // Pan
                    // Could be used for stereo positioning
                    break;
                case 64: // Sustain pedal
                    // Could be used for note sustain
                    break;
            }
            break;
            
        case daisy::MidiMessageType::PitchBend:
            // Handle pitch bend - could be used for fine tuning
            break;
            
        case daisy::MidiMessageType::ProgramChange:
            // Handle program change
            break;
            
        case daisy::MidiMessageType::ChannelPressure:
            // Handle channel pressure
            break;
            
        case daisy::MidiMessageType::PolyphonicKeyPressure:
            // Handle polyphonic key pressure
            break;
            
        default:
            // Other MIDI message types
            break;
    }
}

void OpenChordMidiHandler::SendMidi(const MidiEvent& event) {
    // Send to appropriate output based on source
    switch (event.source) {
        case MidiEvent::Source::TRS_OUT:
            if (trs_midi_initialized_) {
                // Convert to raw MIDI bytes and send
                uint8_t midi_bytes[3];
                size_t byte_count = 0;
                
                switch (event.type) {
                    case daisy::MidiMessageType::NoteOn:
                        midi_bytes[0] = 0x90 | (event.channel & 0x0F);
                        midi_bytes[1] = event.data[0];
                        midi_bytes[2] = event.data[1];
                        byte_count = 3;
                        break;
                    case daisy::MidiMessageType::NoteOff:
                        midi_bytes[0] = 0x80 | (event.channel & 0x0F);
                        midi_bytes[1] = event.data[0];
                        midi_bytes[2] = event.data[1];
                        byte_count = 3;
                        break;
                    case daisy::MidiMessageType::ControlChange:
                        midi_bytes[0] = 0xB0 | (event.channel & 0x0F);
                        midi_bytes[1] = event.data[0];
                        midi_bytes[2] = event.data[1];
                        byte_count = 3;
                        break;
                    default:
                        return; // Unsupported message type
                }
                
                if (byte_count > 0) {
                    trs_midi_.SendMessage(midi_bytes, byte_count);
                }
            }
            break;
            
        case MidiEvent::Source::USB:
            if (usb_midi_initialized_) {
                // Convert to raw MIDI bytes and send
                uint8_t midi_bytes[3];
                size_t byte_count = 0;
                
                switch (event.type) {
                    case daisy::MidiMessageType::NoteOn:
                        midi_bytes[0] = 0x90 | (event.channel & 0x0F);
                        midi_bytes[1] = event.data[0];
                        midi_bytes[2] = event.data[1];
                        byte_count = 3;
                        break;
                    case daisy::MidiMessageType::NoteOff:
                        midi_bytes[0] = 0x80 | (event.channel & 0x0F);
                        midi_bytes[1] = event.data[0];
                        midi_bytes[2] = event.data[1];
                        byte_count = 3;
                        break;
                    case daisy::MidiMessageType::ControlChange:
                        midi_bytes[0] = 0xB0 | (event.channel & 0x0F);
                        midi_bytes[1] = event.data[0];
                        midi_bytes[2] = event.data[1];
                        byte_count = 3;
                        break;
                    default:
                        return; // Unsupported message type
                }
                
                if (byte_count > 0) {
                    usb_midi_.SendMessage(midi_bytes, byte_count);
                }
            }
            break;
            
        default:
            break;
    }
}

void OpenChordMidiHandler::SendMidi(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1) {
    MidiEvent event(type, channel, data0, data1, MidiEvent::Source::TRS_OUT);
    SendMidi(event);
}

float OpenChordMidiHandler::mtof(uint8_t note) const {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
}

void OpenChordMidiHandler::AddToMidiHub(const daisy::MidiEvent& event, MidiEvent::Source source) {
    // Convert Daisy MIDI event to our MidiEvent and add to hub
    MidiEvent our_event(event.type, event.channel, event.data[0], event.data[1], source);
    
    switch (source) {
        case MidiEvent::Source::USB:
            Midi::AddUsbInputEvent(our_event);
            break;
        case MidiEvent::Source::TRS_IN:
            Midi::AddTrsInputEvent(our_event);
            break;
        default:
            break;
    }
}

} // namespace OpenChord
