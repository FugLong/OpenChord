#include "daisy_seed.h"
#include "daisysp.h"
#include "core/config.h"
#include "core/io/io_manager.h"
#include "core/io/input_manager.h"
#include "core/io/storage_manager.h"
#include "core/audio/volume_manager.h"
#include "core/audio/audio_engine.h"
#include "core/midi/midi_handler.h"
// Note: track_interface.h includes plugin_interface.h which includes midi_types.h
// This defines a different MidiEvent than midi_interface.h
// We'll use the track system's MidiEvent (from midi_types.h) via the Track interface
#include "core/tracks/track_interface.h"
#include "core/ui/main_ui.h"
#include "core/ui/ui_manager.h"
#include "core/ui/splash_screen.h"
#include "core/io/button_input_handler.h"
#include "core/io/joystick_input_handler.h"
#include "plugins/input/chord_mapping_input.h"
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

// Track system
Track main_track;
ChordMappingInput* chord_plugin_ptr = nullptr;  // Keep reference for UI

// UI system
SplashScreen splash_screen;
MainUI main_ui;
UIManager ui_manager;

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
    
    // Initialize track system
    main_track.Init();
    main_track.SetName("Track 1");
    
    // Set input modes for chord mapping
    input_manager.SetButtonInputMode(InputMode::MIDI_NOTES);
    input_manager.SetJoystickMode(JoystickMode::CHORD_MAPPING);
    
    // Create and add chord mapping plugin
    auto chord_plugin = std::make_unique<ChordMappingInput>();
    chord_plugin_ptr = chord_plugin.get();  // Keep reference for UI
    chord_plugin->SetInputManager(&input_manager);
    chord_plugin->Init();
    main_track.AddInputPlugin(std::move(chord_plugin));
    
    ExternalLog::PrintLine("Track system initialized with chord mapping");
    
    ExternalLog::PrintLine("Managers initialized");
    
    // Check display status and initialize UI
    DisplayManager* display = io_manager.GetDisplay();
    if (display) {
        if (display->IsHealthy()) {
            ExternalLog::PrintLine("Display: Initialized OK");
            // Give display extra time to stabilize after init
            hw.DelayMs(200);
            
            // Initialize and show splash screen
            splash_screen.Init(display);
            splash_screen.Render();
            ExternalLog::PrintLine("Splash screen displayed");
            
            // Initialize UI Manager (centralized UI coordinator)
            ui_manager.Init(display, &input_manager, &io_manager);
            ui_manager.SetTrack(&main_track);
            ui_manager.SetContext(nullptr);  // Normal mode
            ExternalLog::PrintLine("UI Manager initialized");
            
            // Initialize main UI (default view)
            main_ui.Init(display, &input_manager);
            main_ui.SetTrack(&main_track);
            main_ui.SetChordPlugin(chord_plugin_ptr);
            
            // Register MainUI renderer with UI Manager
            ui_manager.SetMainUIRenderer([](DisplayManager* disp) {
                main_ui.Render(disp);
            });
            ui_manager.SetContentType(UIManager::ContentType::MAIN_UI);
            ExternalLog::PrintLine("Main UI initialized");
            
            #if DEBUG_SCREEN_ENABLED
            // Initialize debug screen system (disabled by default, toggle with INPUT+RECORD)
            debug_screen.Init(display, &input_manager);
            debug_screen.AddView("System", RenderSystemStatusWrapper);
            debug_screen.AddView("Inputs", RenderInputStatusWrapper);
            debug_screen.AddView("Analog", RenderAnalogStatusWrapper);
            debug_screen.AddView("Audio", RenderAudioStatusWrapper);
            debug_screen.AddView("MIDI", RenderMIDIStatusWrapper);
            debug_screen.SetEnabled(false);  // Disabled by default, toggle with button combo
            
            // Register DebugScreen renderer with UI Manager
            ui_manager.SetDebugRenderer([](DisplayManager* disp) {
                debug_screen.Render(disp);
            });
            ExternalLog::PrintLine("Debug screen initialized (disabled by default)");
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
        
        // Update track system (processes input plugins and generates MIDI)
        main_track.Update();
        
        // Get joystick position and route to track
        JoystickInputHandler& joystick = input_manager.GetJoystick();
        float joystick_x, joystick_y;
        joystick.GetPosition(&joystick_x, &joystick_y);
        main_track.HandleJoystick(joystick_x, joystick_y);
        
        // Update splash screen
        splash_screen.Update();
        
        // Show splash screen if it should be displayed
        if (splash_screen.ShouldShow()) {
            splash_screen.Render();
        } else {
            #if DEBUG_SCREEN_ENABLED
            // Update debug screen (handles toggle combo internally)
            debug_screen.Update();
            
            // Update UI Manager content type based on debug screen state
            if (debug_screen.IsEnabled()) {
                ui_manager.SetContentType(UIManager::ContentType::DEBUG);
                ui_manager.SetContext("Debug Mode");
            } else {
                ui_manager.SetContentType(UIManager::ContentType::MAIN_UI);
                ui_manager.SetContext(nullptr);  // Normal mode - show track name
            }
            #endif
            
            // Update UI Manager (coordinates system bar and content area)
            // This handles rendering MainUI or DebugScreen based on content type
            ui_manager.Update();
        }
        
        // Process incoming MIDI events at 1kHz for responsive timing
        midi_handler.ProcessMidi(&audio_engine);
        
        // Generate MIDI from track's input stack and send to MIDI outputs
        // Track uses MidiEvent from midi_types.h (via plugin_interface.h)
        // Note: This is different from MidiHubEvent in midi_interface.h
        // Use fully qualified name to avoid ambiguity with daisy::MidiEvent
        ::OpenChord::MidiEvent midi_events[64];
        size_t midi_event_count = 0;
        main_track.GenerateMIDI(midi_events, &midi_event_count, 64);
        
        // Send generated MIDI events to USB and TRS MIDI outputs
        // Convert from plugin system's MidiEvent (midi_types.h) to handler
        for (size_t i = 0; i < midi_event_count; i++) {
            const ::OpenChord::MidiEvent& event = midi_events[i];
            // Convert MidiEvent::Type (from midi_types.h) to daisy::MidiMessageType
            // midi_types.h uses: NOTE_ON = 0x90, NOTE_OFF = 0x80
            daisy::MidiMessageType msg_type;
            if (event.type == static_cast<uint8_t>(::OpenChord::MidiEvent::Type::NOTE_ON)) {
                msg_type = daisy::MidiMessageType::NoteOn;
            } else if (event.type == static_cast<uint8_t>(::OpenChord::MidiEvent::Type::NOTE_OFF)) {
                msg_type = daisy::MidiMessageType::NoteOff;
            } else {
                msg_type = static_cast<daisy::MidiMessageType>(event.type);
            }
            midi_handler.SendMidi(msg_type, event.channel, event.data1, event.data2);
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