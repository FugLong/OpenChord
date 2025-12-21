#include "daisy_seed.h"
#include "daisysp.h"
#include "core/io/io_manager.h"
#include "core/io/digital_manager.h"
#include "core/io/storage_manager.h"
#include "core/audio/volume_manager.h"
#include "core/audio/audio_engine.h"
#include "core/midi/midi_handler.h"
#include "core/midi/midi_interface.h"

using namespace daisy;
using namespace OpenChord;

// Debug mode flag (matches midi_handler.cpp DEBUG_MODE)
#define DEBUG_MODE false

// Use external USB logger for serial output (pins 36-37)
using ExternalLog = Logger<LOGGER_EXTERNAL>;


// Global hardware instance
DaisySeed hw;

// Global instances
VolumeManager volume_mgr;
AudioEngine audio_engine;
IOManager io_manager;
OpenChordMidiHandler midi_handler;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    audio_engine.ProcessAudio(in, out, size);
}

int main(void) {
    // 1) Initialize hardware
    hw.Init();
    
    // 2) Start logging for printing over serial (using external USB pins)
    // Using LOGGER_EXTERNAL for external USB pins (D29/D30 = pins 36-37)
    ExternalLog::StartLog(false); //True to block the program until serial is connected, false to continue immediately
    
    // Debug mode: Give time to connect to serial before starting
    #if DEBUG_MODE
    hw.DelayMs(3000);  // 3 second delay to allow serial connection
    #endif
    
    ExternalLog::PrintLine("OpenChord firmware booting...");
    
    // 3) Configure audio
    hw.SetAudioBlockSize(4);
    ExternalLog::PrintLine("Audio configured");
    
    // 4) Initialize managers
    io_manager.Init(&hw);
    
    volume_mgr.SetIO(&io_manager);
    audio_engine.Init(&hw);
    audio_engine.SetVolumeManager(&volume_mgr);
    
    // Initialize MIDI handler
    midi_handler.Init(&hw);
    ExternalLog::PrintLine("MIDI handler initialized");
    
    ExternalLog::PrintLine("Managers initialized");
    ExternalLog::PrintLine("Audio engine ready");
    
    // 6) Start audio
    hw.StartAudio(AudioCallback);
    ExternalLog::PrintLine("Audio started");
    
    hw.DelayMs(100);
    
    // Test SD card
    StorageManager* storage = io_manager.GetStorage();
    if (storage) {
        if (storage->TestCard()) {
            ExternalLog::PrintLine("SD card: Test PASSED");
        } else {
            ExternalLog::PrintLine("SD card: Test FAILED (not mounted or filesystem error)");
        }
    }
    
    ExternalLog::PrintLine("System initialized OK");
    
    // 7) Main loop - 1kHz constant timing for predictable behavior
    while(1) {
        io_manager.Update();
        volume_mgr.Update();  // Update volume manager to get latest ADC values
        
        // Check button matrix and generate MIDI events
        DigitalManager* digital = io_manager.GetDigital();
        if (digital) {
            // Check each button position for press/release
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 4; col++) {
                    // Skip unused position (Row 1, Col 3)
                    if (row == 1 && col == 3) continue;
                    
                    // Calculate MIDI note: sequential from 60 (C4) for all 11 keys
                    // Layout: bottom-to-top, left-to-right, so bottom-left (2,0) = 60
                    // Row 2: cols 0,1,2,3 = notes 60,61,62,63
                    // Row 1: cols 0,1,2 = notes 64,65,66 (skip col 3)
                    // Row 0: cols 0,1,2,3 = notes 67,68,69,70
                    uint8_t midi_note = 60;
                    if (row == 2) {
                        midi_note = 60 + col; // 60-63
                    } else if (row == 1) {
                        midi_note = 64 + col; // 64-66 (col 3 skipped)
                    } else { // row == 0
                        midi_note = 67 + col; // 67-70
                    }
                    
                    // Check for button press (just pressed) - WasKeyPressed() detects edge
                    // This returns true only for ONE scan cycle when button transitions from not pressed to pressed
                    if (digital->WasKeyPressed(row, col)) {
                        // Send MIDI Note On
                        Midi::AddGeneratedEvent(
                            daisy::MidiMessageType::NoteOn,
                            0,  // Channel 0
                            midi_note,
                            100 // Velocity
                        );
                    }
                    
                    // Check for button release (just released)
                    // Track previous state to detect release edge
                    static bool prev_key_states[3][4] = {false};
                    bool current_pressed = digital->IsKeyPressed(row, col);
                    if (prev_key_states[row][col] && !current_pressed) {
                        // Key was pressed, now released
                        Midi::AddGeneratedEvent(
                            daisy::MidiMessageType::NoteOff,
                            0,  // Channel 0
                            midi_note,
                            0   // Velocity (0 for NoteOff)
                        );
                    }
                    prev_key_states[row][col] = current_pressed;
                }
            }
        }
        
        // Process MIDI events at 1kHz for responsive timing
        midi_handler.ProcessMidi(&audio_engine);
        
        // Forward generated MIDI events (from button matrix) to TRS MIDI output for testing
        const std::vector<OpenChord::MidiEvent>& generated_events = Midi::GetGeneratedEvents();
        for (const OpenChord::MidiEvent& event : generated_events) {
            midi_handler.SendMidi(event.type, event.channel, event.data[0], event.data[1]);
        }
        
        audio_engine.ProcessMidi();
        
        // Debug: Check volume manager status every second
        static uint32_t debug_counter = 0;
        static bool midi_enabled_printed = false;
        
        // Print MIDI enabled status once after initialization
        if (!midi_enabled_printed && debug_counter > 100) {
            ExternalLog::PrintLine("MIDI Enabled: TRS=%s, USB=%s", 
                        midi_handler.IsTrsInitialized() ? "YES" : "NO",
                        midi_handler.IsUsbInitialized() ? "YES" : "NO");
            midi_enabled_printed = true;
        }
        
        if (++debug_counter % 1000 == 0) { // Every 1000 iterations = ~1 second
            // Commented out verbose debug logging
            // const VolumeData& volume_data = volume_mgr.GetVolumeData();
            // hw.PrintLine("Volume Debug - Raw: %.4f, Amp: %.4f, Line: %.4f, Changed: %s", 
            //            volume_data.raw_adc, volume_data.amplitude, volume_data.line_level,
            //            volume_mgr.HasVolumeChanged() ? "YES" : "NO");
            
            // Debug: Check what analog manager actually has
            // float analog_raw = io_manager.GetAnalog()->GetADCValue(0);
            // hw.PrintLine("Analog Debug - Raw: %.4f", analog_raw);
        }
        
        if (volume_mgr.HasVolumeChanged()) {
            // Commented out verbose volume change logging
            // const VolumeData& volume_data = volume_mgr.GetVolumeData();
            // hw.PrintLine("Volume changed - Raw: %.3f, Amp: %.3f, Line: %.3f", 
            //            volume_data.raw_adc, volume_data.amplitude, volume_data.line_level);
            volume_mgr.ClearChangeFlag();
        }
        
        // LED heartbeat
        static uint32_t heartbeat = 0;
        if (++heartbeat % 1000 == 0) { // Every 1000 iterations = ~1 second
            hw.SetLed(true);   // Turn LED on
            hw.DelayMs(50);    // Keep on for 50ms
            hw.SetLed(false);  // Turn LED off
        }
        
        // 1kHz constant timing - 1ms delay for predictable behavior
        hw.DelayMs(1);
    }
} 