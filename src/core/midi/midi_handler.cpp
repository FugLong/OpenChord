#include "midi_handler.h"
#include <cmath>

// Debug mode flag - set to true to disable USB MIDI for serial debugging
#define DEBUG_MODE false

namespace OpenChord {

// MIDI buffer configuration
static constexpr size_t kMidiBufferSize = 256;

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
    usb_config.transport_config.periph = daisy::MidiUsbTransport::Config::INTERNAL;
    usb_config.transport_config.tx_retry_count = 3;
    
    usb_midi_.Init(usb_config);
    usb_midi_.StartReceive();
    usb_midi_initialized_ = true;
    if (hw_) hw_->PrintLine("USB MIDI initialized");
    #else
    usb_midi_initialized_ = false;
    if (hw_) hw_->PrintLine("Debug mode: USB MIDI disabled for serial debugging");
    #endif
    
    // Initialize TRS MIDI using UartHandler directly (like the working Daisy example)
    daisy::UartHandler::Config uart_config;
    
    // Configure UART4 properly using Daisy's actual UART configuration method
    uart_config.baudrate = 31250;  // Standard MIDI baud rate
    uart_config.periph = daisy::UartHandler::Config::Peripheral::UART_4;
    uart_config.stopbits = daisy::UartHandler::Config::StopBits::BITS_1;
    uart_config.parity = daisy::UartHandler::Config::Parity::NONE;
    uart_config.mode = daisy::UartHandler::Config::Mode::TX_RX;
    uart_config.wordlength = daisy::UartHandler::Config::WordLength::BITS_8;
    
    // Use the correct pin configuration method from the working example
    uart_config.pin_config.rx = daisy::Pin(daisy::PORTB, 8);  // D11 = PB8 = Physical Pin 12
    uart_config.pin_config.tx = daisy::Pin(daisy::PORTB, 9);  // D12 = PB9 = Physical Pin 13
    
    if (hw_) {
        hw_->PrintLine("UART4 Pin Config: RX=PB8 (D11, Pin 12), TX=PB9 (D12, Pin 13)");
        hw_->PrintLine("Using external pull-up resistor on RX pin");
    }
    
    // Initialize UART4 with proper configuration
    trs_midi_initialized_ = (trs_uart_.Init(uart_config) == daisy::UartHandler::Result::OK);
    
    if (hw_) {
        hw_->PrintLine("TRS MIDI initialized on UART4 (Pins 12-13)");
        hw_->PrintLine("UART4 Config: Baud=%d, RX=PB8, TX=PB9, Init=%s", 
                      uart_config.baudrate, trs_midi_initialized_ ? "OK" : "FAILED");
    }
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
    
    // MIDI parsing state
    static uint8_t midi_buffer[3];
    static uint8_t buffer_pos = 0;
    static uint8_t expected_bytes = 0;
    
    
    // Non-blocking UART receive
    uint8_t byte;
    int bytes_received = trs_uart_.PollReceive(&byte, 1, 0);
    
    if (bytes_received > 0) {
        // Process the received MIDI byte
        if (byte & 0x80) {  // Status byte (start of message)
            buffer_pos = 0;
            midi_buffer[buffer_pos++] = byte;
            
            // Determine expected data bytes based on message type
            uint8_t message_type = byte & 0xF0;
            switch (message_type) {
                case 0x80: case 0x90: case 0xA0: case 0xB0: case 0xE0:  // Note Off, Note On, Poly Pressure, Control Change, Pitch Bend
                    expected_bytes = 3;
                    break;
                case 0xC0: case 0xD0:  // Program Change, Channel Pressure
                    expected_bytes = 2;
                    break;
                case 0xF0:  // System messages
                    if (byte == 0xF0) expected_bytes = 0;  // SysEx - variable length
                    else if (byte == 0xF1 || byte == 0xF3) expected_bytes = 2;
                    else if (byte == 0xF2) expected_bytes = 3;
                    else expected_bytes = 1;
                    break;
                default:
                    expected_bytes = 0;
                    break;
            }
        } else if (buffer_pos > 0 && buffer_pos < expected_bytes) {  // Data byte
            midi_buffer[buffer_pos++] = byte;
        }
        
        // Process complete MIDI message
        if (buffer_pos >= expected_bytes && expected_bytes > 0) {
            // Convert to Daisy MidiEvent and add to MidiHub
            daisy::MidiEvent event;
            event.type = static_cast<daisy::MidiMessageType>(midi_buffer[0] & 0xF0);
            event.channel = midi_buffer[0] & 0x0F;
            event.data[0] = (expected_bytes >= 2) ? midi_buffer[1] : 0;
            event.data[1] = (expected_bytes >= 3) ? midi_buffer[2] : 0;
            
            // Debug: Print TRS MIDI events (only for Note On/Off and CC)
            if (hw_ && (event.type == daisy::MidiMessageType::NoteOn || 
                       event.type == daisy::MidiMessageType::NoteOff ||
                       event.type == daisy::MidiMessageType::ControlChange)) {
                hw_->PrintLine("TRS MIDI: Type=0x%02X, Ch=%d, Data=[%d,%d]", 
                              static_cast<uint8_t>(event.type), event.channel, 
                              event.data[0], event.data[1]);
            }
            
            AddToMidiHub(event, MidiEvent::Source::TRS_IN);
            
            // Reset buffer
            buffer_pos = 0;
            expected_bytes = 0;
        }
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
        
        // Convert to raw MIDI bytes
        ConvertToMidiBytes(event, midi_bytes, &byte_count);
        
        if (byte_count > 0) {
            trs_uart_.BlockingTransmit(midi_bytes, byte_count, 1000);
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

float OpenChordMidiHandler::mtof(uint8_t note) const {
    return 440.0f * powf(2.0f, (note - 69) / 12.0f);
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
            *size = 0;
            break;
    }
}

} // namespace OpenChord
