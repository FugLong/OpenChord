#pragma once

#include "daisy_seed.h"
#include "hid/midi.h"
#include "midi_interface.h"

namespace OpenChord {

// TRS MIDI handler for DIN-5 MIDI input/output
class TrsMidiHandler {
private:
    daisy::MidiUartHandler midi_;
    bool midi_initialized_;
    
    // Configuration
    daisy::Pin rx_pin_;
    daisy::Pin tx_pin_;
    daisy::UartHandler::Config::Peripheral uart_periph_;
    
public:
    TrsMidiHandler() : midi_initialized_(false) {}
    
    // Initialize TRS MIDI with specific pins and UART peripheral
    void Init(daisy::Pin rx_pin, daisy::Pin tx_pin, 
              daisy::UartHandler::Config::Peripheral uart_periph = daisy::UartHandler::Config::Peripheral::UART_4);
    
    // Process incoming TRS MIDI and add to MidiHub
    void ProcessMidi();
    
    // Send MIDI events to TRS output
    void SendMidi(const MidiEvent& event);
    void SendMidi(daisy::MidiMessageType type, uint8_t channel, uint8_t data0, uint8_t data1);
    
    // Send raw MIDI bytes
    void SendRawMidi(uint8_t* data, size_t size);
    
    // Status queries
    bool IsInitialized() const { return midi_initialized_; }
    bool IsRxActive();
    
    // Configuration
    void SetRxPin(daisy::Pin pin) { rx_pin_ = pin; }
    void SetTxPin(daisy::Pin pin) { tx_pin_ = pin; }
    void SetUartPeripheral(daisy::UartHandler::Config::Peripheral periph) { uart_periph_ = periph; }
    
private:
    // Convert Daisy MidiMessageType to raw MIDI bytes for transmission
    void ConvertToMidiBytes(const MidiEvent& event, uint8_t* bytes, size_t* size);
};

} // namespace OpenChord
