#include "daisy_seed.h"
#include "daisysp.h"
#include "../include/midi_interface.h"
#include "../include/volume_manager.h"
#include "../include/audio_engine.h"
#include "../include/midi_handler.h"

using namespace daisy;
using namespace OpenChord;

// Global hardware instance (needed for AudioCallback)
daisy::DaisySeed hw;

// Global instances
VolumeManager volume_mgr;
AudioEngine audio_engine;
OpenChordMidiHandler midi_handler;
IO io_system;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    // Update volume manager
    volume_mgr.Update();
    
    // Generate audio using the proper AudioEngine interface
    // The AudioEngine will handle volume control internally via the volume manager
    static float audio_buffer[256 * 2]; // Stereo buffer for audio processing
    
    audio_engine.ProcessAudio(nullptr, audio_buffer, size);
    
    // Copy the processed audio to the output buffers
    for (size_t i = 0; i < size; i++) {
        out[0][i] = audio_buffer[i * 2];     // Left channel
        out[1][i] = audio_buffer[i * 2 + 1]; // Right channel
    }
}

int main(void) {
    // Initialize hardware
    hw.Init();
    hw.SetAudioBlockSize(4);
    
    // Initialize IO system
    io_system.Init(&hw);
    
    // Configure ADC 0 for pin 22 (volume pot)
    io_system.ConfigureADC(0, seed::A0);
    
    // Initialize volume manager and connect to IO system
    volume_mgr.SetIO(&io_system);
    
    // Start logging (non-blocking)
    hw.StartLog(false);
    hw.PrintLine("OpenChord Firmware Starting...");
    hw.PrintLine("Initializing USB MIDI...");
    
    // Initialize MIDI handler (handles both USB and TRS)
    midi_handler.Init(&hw);
    
    hw.PrintLine("MIDI system initialized (USB + TRS on pins 12/13)");
    
    // Initialize audio engine
    audio_engine.Init(&hw);
    
    // Set up volume manager integration
    audio_engine.SetVolumeManager(&volume_mgr);
    
    // Optional: Adjust ADSR envelope for different piano sounds
    // audio_engine.SetAttackTime(5.0f);      // 5ms attack (bright piano)
    // audio_engine.SetDecayTime(50.0f);      // 50ms decay
    // audio_engine.SetSustainLevel(80.0f);   // 80% sustain
    // audio_engine.SetReleaseTime(200.0f);   // 200ms release
    
    // Set up audio callback and start audio
    hw.StartAudio(AudioCallback);
    
    // Start ADC for volume pot
    hw.adc.Start();
    
    hw.PrintLine("System initialized successfully!");
    hw.PrintLine("Volume Control System Active");
    hw.PrintLine("Audio Engine: MIDI-controlled Oscillator with Daisy ADSR Envelope");
    hw.PrintLine("ADSR: Attack=10ms, Decay=100ms, Sustain=70%%, Release=300ms");
    hw.PrintLine("MIDI System: Unified USB + TRS handler active");
    hw.PrintLine("TRS MIDI: Pins 12 (RX) and 13 (TX) using UART4");
    hw.PrintLine("MidiHub: Global MIDI data access enabled");
    hw.PrintLine("---");
    
    // Main loop
    while (1) {
        // Update IO system (ADC, buttons, encoder, etc.)
        io_system.Update();
        
        // Update volume manager
        volume_mgr.Update();
        
        // Process MIDI input
        midi_handler.ProcessMidi(&audio_engine);
        
        // Example: Access global MIDI data from anywhere
        const auto& usb_events = Midi::GetUsbInputEvents();
        const auto& trs_input_events = Midi::GetTrsInputEvents();
        const auto& trs_output_events = Midi::GetTrsOutputBuffer();
        const auto& generated_events = Midi::GetGeneratedEvents();
        const auto& combined_events = Midi::GetCombinedEvents();
        
        // Debug output when volume changes
        if (volume_mgr.HasVolumeChanged()) {
            const VolumeData& volume_data = volume_mgr.GetVolumeData();
            
            // Convert to integers for reliable output
            int raw_int = (int)(volume_data.raw_adc * 1000);
            int scaled_int = (int)(volume_data.scaled_volume * 1000);
            
            hw.PrintLine("Raw: %d (div1000)", raw_int);
            hw.PrintLine("Scaled: %d (div1000)", scaled_int);
            hw.PrintLine("MIDI Events - USB: %d, TRS_IN: %d, TRS_OUT: %d, Generated: %d, Combined: %d", 
                       (int)usb_events.size(), (int)trs_input_events.size(), 
                       (int)trs_output_events.size(), (int)generated_events.size(), 
                       (int)combined_events.size());
            hw.PrintLine("---");
            
            volume_mgr.ClearChangeFlag();
        }
        
        // Small delay to prevent overwhelming the serial output
        hw.DelayMs(10);
    }
} 