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
#include "core/ui/menu_manager.h"
#include "core/ui/settings_manager.h"
#include "core/midi/octave_shift.h"
#include "core/io/button_input_handler.h"
#include "core/io/joystick_input_handler.h"
#include "plugins/input/chord_mapping_input.h"
#include "plugins/input/chromatic_input.h"
#include "plugins/input/drum_pad_input.h"
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

// Octave shift system
OctaveShift octave_shift;

// Track system
Track main_track;
ChordMappingInput* chord_plugin_ptr = nullptr;  // Keep reference for UI
ChromaticInput* chromatic_plugin_ptr = nullptr;  // Keep reference for octave shift

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
    
    // Microphone passthrough disabled by default
    audio_engine.SetMicPassthroughEnabled(false);
    
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
    // Add chord mapping plugin (higher priority)
    auto chord_plugin = std::make_unique<ChordMappingInput>();
    chord_plugin_ptr = chord_plugin.get();  // Keep reference for UI
    chord_plugin->SetInputManager(&input_manager);
    // Pass function to check if octave UI is active (so chord mapping doesn't process joystick when octave UI is active)
    // Note: This will be set after UI Manager is initialized
    chord_plugin->Init();
    main_track.AddInputPlugin(std::move(chord_plugin));
    
    // Add drum pad plugin (exclusive, medium priority)
    auto drum_pad_plugin = std::make_unique<DrumPadInput>();
    drum_pad_plugin->SetInputManager(&input_manager);
    drum_pad_plugin->Init();
    main_track.AddInputPlugin(std::move(drum_pad_plugin));
    
    // Add chromatic input plugin (lower priority, fallback when no other input is active)
    auto chromatic_plugin = std::make_unique<ChromaticInput>();
    chromatic_plugin_ptr = chromatic_plugin.get();
    chromatic_plugin->SetInputManager(&input_manager);
    chromatic_plugin->SetOctaveShift(&octave_shift);
    chromatic_plugin->SetTrack(&main_track);  // Allow it to check for other active plugins
    chromatic_plugin->Init();
    main_track.AddInputPlugin(std::move(chromatic_plugin));
    
    ExternalLog::PrintLine("Track system initialized with chord mapping, drum pad, and chromatic input");
    
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
            ui_manager.SetOctaveShift(&octave_shift);  // Provide octave shift for octave UI
            ui_manager.SetContext(nullptr);  // Normal mode
            ExternalLog::PrintLine("UI Manager initialized");
            
            // Initialize main UI (default view)
            main_ui.Init(display, &input_manager);
            main_ui.SetTrack(&main_track);
            main_ui.SetChordPlugin(chord_plugin_ptr);
            main_ui.SetChromaticPlugin(chromatic_plugin_ptr);
            
            // Register MainUI renderer with UI Manager
            ui_manager.SetMainUIRenderer([](DisplayManager* disp) {
                main_ui.Render(disp);
            });
            ui_manager.SetContentType(UIManager::ContentType::MAIN_UI);
            
            // Set octave UI check callback for chord plugin (after UI Manager is initialized)
            if (chord_plugin_ptr) {
                chord_plugin_ptr->SetOctaveUICheckCallback([]() -> bool {
                    return ui_manager.IsOctaveUIActive();
                });
            }
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
            
            // Check if we should show debug mode (takes priority over menu)
            bool debug_enabled = debug_screen.IsEnabled();
            ui_manager.SetDebugMode(debug_enabled);
            
            if (debug_enabled) {
                ui_manager.SetContext("Debug Mode");
            } else {
                ui_manager.SetContext(nullptr);  // Clear debug context
            }
            
            if (!debug_enabled)
            #endif
            {
                // Handle menu system (button presses and navigation)
                MenuManager* menu_mgr = ui_manager.GetMenuManager();
                SettingsManager* settings_mgr = ui_manager.GetSettingsManager();
                
                // Handle button presses to open/close menus
                if (input_manager.GetButtons().WasSystemButtonPressed(SystemButton::INPUT)) {
                    if (menu_mgr) {
                        if (menu_mgr->GetCurrentMenuType() == MenuManager::MenuType::INPUT_STACK) {
                            menu_mgr->CloseMenu();
                        } else {
                            menu_mgr->OpenInputStackMenu();
                            // Restore settings plugin if it exists (menu memory)
                            if (settings_mgr && settings_mgr->GetPlugin()) {
                                menu_mgr->SetCurrentSettingsPlugin(settings_mgr->GetPlugin());
                            }
                        }
                    }
                }
                
                if (input_manager.GetButtons().WasSystemButtonPressed(SystemButton::INSTRUMENT)) {
                    if (menu_mgr) {
                        if (menu_mgr->GetCurrentMenuType() == MenuManager::MenuType::INSTRUMENT) {
                            menu_mgr->CloseMenu();
                        } else {
                            menu_mgr->OpenInstrumentMenu();
                        }
                    }
                }
                
                if (input_manager.GetButtons().WasSystemButtonPressed(SystemButton::FX)) {
                    if (menu_mgr) {
                        if (menu_mgr->GetCurrentMenuType() == MenuManager::MenuType::FX) {
                            menu_mgr->CloseMenu();
                        } else {
                            menu_mgr->OpenFXMenu();
                        }
                    }
                }
                
                // Handle menu/settings navigation (centralized in MenuManager)
                if (menu_mgr && menu_mgr->IsOpen()) {
                    uint32_t current_time = hw.system.GetNow();
                    menu_mgr->UpdateMenuInput(settings_mgr, &io_manager, current_time);
                    
                    // Update context in system bar
                    ui_manager.SetContext(menu_mgr->GetContextName());
                } else {
                    // Normal mode - handle octave UI (stick click)
                    if (!menu_mgr || !menu_mgr->IsOpen()) {
                        static bool prev_joystick_button_normal = false;
                        bool joystick_button = false;
                        if (io_manager.GetDigital()) {
                            joystick_button = io_manager.GetDigital()->WasJoystickButtonPressed();
                        }
                        
                        if (joystick_button && !prev_joystick_button_normal) {
                            // Toggle octave UI via UI Manager (which manages its own state)
                            if (ui_manager.IsOctaveUIActive()) {
                                ui_manager.DeactivateOctaveUI();
                            } else {
                                ui_manager.ActivateOctaveUI();
                            }
                        }
                        prev_joystick_button_normal = joystick_button;
                        
                        // Update octave UI with joystick input
                        if (ui_manager.IsOctaveUIActive()) {
                            float joystick_x, joystick_y;
                            input_manager.GetJoystick().GetPosition(&joystick_x, &joystick_y);
                            uint32_t current_time = hw.system.GetNow();
                            ui_manager.UpdateOctaveUI(joystick_x, current_time);
                        }
                    } else {
                        // Menu is open, deactivate octave UI if it was active
                        if (ui_manager.IsOctaveUIActive()) {
                            ui_manager.DeactivateOctaveUI();
                        }
                    }
                    
                    ui_manager.SetContentType(UIManager::ContentType::MAIN_UI);
                    ui_manager.SetContext(nullptr);  // Normal mode - show track name
                }
            }
            
            // Update UI Manager (handles all state updates and rendering)
            // UI Manager owns the display lifecycle and coordinates all rendering
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
        // Apply octave shift and convert from plugin system's MidiEvent (midi_types.h) to handler
        for (size_t i = 0; i < midi_event_count; i++) {
            ::OpenChord::MidiEvent event = midi_events[i];  // Copy to apply shift
            
            // Apply octave shift to note messages
            if (event.type == static_cast<uint8_t>(::OpenChord::MidiEvent::Type::NOTE_ON) ||
                event.type == static_cast<uint8_t>(::OpenChord::MidiEvent::Type::NOTE_OFF)) {
                event.data1 = octave_shift.ApplyShift(event.data1);
            }
            
            // Convert MidiEvent::Type (from midi_types.h) to daisy::MidiMessageType
            // midi_types.h uses: NOTE_ON = 0x90, NOTE_OFF = 0x80, PITCH_BEND = 0xE0, CONTROL_CHANGE = 0xB0
            daisy::MidiMessageType msg_type;
            if (event.type == static_cast<uint8_t>(::OpenChord::MidiEvent::Type::NOTE_ON)) {
                msg_type = daisy::MidiMessageType::NoteOn;
            } else if (event.type == static_cast<uint8_t>(::OpenChord::MidiEvent::Type::NOTE_OFF)) {
                msg_type = daisy::MidiMessageType::NoteOff;
            } else if (event.type == static_cast<uint8_t>(::OpenChord::MidiEvent::Type::PITCH_BEND)) {
                msg_type = daisy::MidiMessageType::PitchBend;
            } else if (event.type == static_cast<uint8_t>(::OpenChord::MidiEvent::Type::CONTROL_CHANGE)) {
                msg_type = daisy::MidiMessageType::ControlChange;
            } else {
                msg_type = static_cast<daisy::MidiMessageType>(event.type);
            }
            midi_handler.SendMidi(msg_type, event.channel, event.data1, event.data2);
        }
        
        audio_engine.ProcessMidi();
        
        // Print MIDI enabled status once after initialization
        static bool midi_enabled_printed = false;
        static uint32_t init_counter = 0;
        if (!midi_enabled_printed && ++init_counter > 100) {
            ExternalLog::PrintLine("MIDI Enabled: TRS=%s, USB=%s", 
                        midi_handler.IsTrsInitialized() ? "YES" : "NO",
                        midi_handler.IsUsbInitialized() ? "YES" : "NO");
            midi_enabled_printed = true;
        }
        
        if (volume_mgr.HasVolumeChanged()) {
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