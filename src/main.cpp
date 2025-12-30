#include "daisy_seed.h"
#include "daisysp.h"
#include "core/config.h"
#include "core/io/io_manager.h"
#include "core/io/input_manager.h"
#include "core/io/storage_manager.h"
#include "core/audio/volume_manager.h"
#include "core/audio/audio_engine.h"
#include "core/midi/midi_handler.h"
#include "core/midi/midi_interface.h"
#if DEBUG_SCREEN_ENABLED
#include "core/ui/debug_screen.h"
#include "core/ui/debug_views.h"
#endif

using namespace daisy;
using namespace OpenChord;

// Use external USB logger for serial output (pins 36-37)
using ExternalLog = Logger<LOGGER_EXTERNAL>;


// Global hardware instance
DaisySeed hw;

// Global instances
VolumeManager volume_mgr;
AudioEngine audio_engine;
IOManager io_manager;
OpenChordMidiHandler midi_handler;
InputManager input_manager;

#if DEBUG_SCREEN_ENABLED
DebugScreen debug_screen;

// Wrapper functions to pass global instances to render functions
void RenderSystemStatusWrapper(DisplayManager* display) {
    RenderSystemStatus(display, &io_manager);
}

void RenderInputStatusWrapper(DisplayManager* display) {
    RenderInputStatus(display, &input_manager, &io_manager);
}

void RenderAnalogStatusWrapper(DisplayManager* display) {
    RenderAnalogStatus(display, &io_manager);
}

void RenderAudioStatusWrapper(DisplayManager* display) {
    RenderAudioStatus(display, &audio_engine, &volume_mgr);
}

void RenderMIDIStatusWrapper(DisplayManager* display) {
    RenderMIDIStatus(display, &midi_handler);
}
#endif

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
    
    // Initialize unified input manager (coordinates buttons, joystick, encoder)
    input_manager.Init(&io_manager);
    
    volume_mgr.SetIO(&io_manager);
    audio_engine.Init(&hw);
    audio_engine.SetVolumeManager(&volume_mgr);
    
    // Enable microphone passthrough for wiring test (temporary)
    audio_engine.SetMicPassthroughEnabled(false);
    ExternalLog::PrintLine("Microphone passthrough enabled (temporary test)");
    
    // Initialize MIDI handler
    midi_handler.Init(&hw);
    ExternalLog::PrintLine("MIDI handler initialized");
    
    ExternalLog::PrintLine("Managers initialized");
    
    // Check display status and initialize debug screen if enabled
    DisplayManager* display = io_manager.GetDisplay();
    if (display) {
        if (display->IsHealthy()) {
            ExternalLog::PrintLine("Display: Initialized OK");
            // Give display extra time to stabilize after init
            hw.DelayMs(200);
            
            #if DEBUG_SCREEN_ENABLED
            // Initialize debug screen system
            debug_screen.Init(display, &input_manager);
            debug_screen.AddView("System", RenderSystemStatusWrapper);
            debug_screen.AddView("Inputs", RenderInputStatusWrapper);
            debug_screen.AddView("Analog", RenderAnalogStatusWrapper);
            debug_screen.AddView("Audio", RenderAudioStatusWrapper);
            debug_screen.AddView("MIDI", RenderMIDIStatusWrapper);
            debug_screen.SetEnabled(true);
            ExternalLog::PrintLine("Debug screen initialized");
            #else
            // Test display with simple pattern if debug screen disabled
            display->TestDisplay();
            ExternalLog::PrintLine("Display: Test pattern sent");
            #endif
        } else {
            ExternalLog::PrintLine("Display: Initialization FAILED");
        }
    }
    
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
        input_manager.Update();       // Update unified input manager (handles all inputs)
        volume_mgr.Update();          // Update volume manager to get latest ADC values
        
        #if DEBUG_SCREEN_ENABLED
        debug_screen.Update();        // Update debug screen (renders at configured interval)
        #endif
        
        // Handle button inputs using the unified input manager
        // For now, we'll handle MIDI note generation for musical buttons
        // System buttons will be handled in future UI/system code
        
        ButtonInputHandler& buttons = input_manager.GetButtons();
        
        // Process musical buttons (7 keys: 4 white + 3 black)
        // Map to MIDI notes: White keys = 60-63, Black keys = 64-66
        static bool prev_musical_states[7] = {false};
        
        for (int i = 0; i < static_cast<int>(MusicalButton::COUNT); i++) {
            MusicalButton button = static_cast<MusicalButton>(i);
            bool current_pressed = buttons.IsMusicalButtonPressed(button);
            bool was_pressed = buttons.WasMusicalButtonPressed(button);
            
            // Calculate MIDI note based on button
            // White keys (0-3): notes 60-63
            // Black keys (4-6): notes 64-66
            uint8_t midi_note = (i < 4) ? (60 + i) : (64 + (i - 4));
            
            // Handle press event
            if (was_pressed || (!prev_musical_states[i] && current_pressed)) {
                Midi::AddGeneratedEvent(
                    daisy::MidiMessageType::NoteOn,
                    0,  // Channel 0
                    midi_note,
                    100 // Velocity
                );
            }
            
            // Handle release event
            if (prev_musical_states[i] && !current_pressed) {
                Midi::AddGeneratedEvent(
                    daisy::MidiMessageType::NoteOff,
                    0,  // Channel 0
                    midi_note,
                    0   // Velocity (0 for NoteOff)
                );
            }
            
            prev_musical_states[i] = current_pressed;
        }
        
        // Process system buttons (4 keys on top row)
        // These will be used for UI/system control in the future
        // For now, we just track their state but don't generate MIDI
        // TODO: Implement system button handlers for:
        // - INPUT: Input selection (also play/pause loop)
        // - INSTRUMENT: Instrument selection/options
        // - FX: FX selection/options
        // - RECORD: Record start/stop, loop settings
        
        // Joystick control code removed - pins and initialization remain for future use
        
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