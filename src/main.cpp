#include "daisy_seed.h"
#include "daisysp.h"
#include "core/io/io_manager.h"
#include "core/audio/volume_manager.h"
#include "core/audio/audio_engine.h"
#include "core/midi/midi_handler.h"

using namespace daisy;
using namespace OpenChord;

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
    
    // 2) Start logging for printing over serial
    hw.StartLog(false); //True to block the program until serial is connected, false to continue immediately
    hw.PrintLine("OpenChord firmware booting...");
    
    // 3) Configure audio
    hw.SetAudioBlockSize(4);
    hw.PrintLine("Audio configured");
    
    // 4) Initialize managers
    io_manager.Init(&hw);
    
    volume_mgr.SetIO(&io_manager);
    audio_engine.Init(&hw);
    audio_engine.SetVolumeManager(&volume_mgr);
    
    // Initialize MIDI handler
    midi_handler.Init(&hw);
    hw.PrintLine("MIDI handler initialized");
    
    hw.PrintLine("Managers initialized");
    hw.PrintLine("Audio engine ready");
    
    // 6) Start audio
    hw.StartAudio(AudioCallback);
    hw.PrintLine("Audio started");
    
    hw.DelayMs(100);
    hw.PrintLine("System initialized OK");
    
    // 7) Main loop
    while(1) {
        io_manager.Update();
        volume_mgr.Update();  // Update volume manager to get latest ADC values
        
        // Process MIDI events
        midi_handler.ProcessMidi(&audio_engine);
        
        // Debug: Check volume manager status every second
        static uint32_t debug_counter = 0;
        if (++debug_counter % 100 == 0) { // Every 100 iterations = ~1 second
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
        if (++heartbeat % 100 == 0) { // Every 100 iterations = ~1 second
            hw.SetLed(true);   // Turn LED on
            hw.DelayMs(50);    // Keep on for 50ms
            hw.SetLed(false);  // Turn LED off
        }
        
        hw.DelayMs(10);  // 10ms delay = 100Hz loop rate
    }
} 