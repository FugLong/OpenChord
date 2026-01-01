#include "daisy_seed.h"
#include "daisysp.h"
#include "core/config.h"
#include "core/io/io_manager.h"
#include "core/io/input_manager.h"
#include "core/io/storage_manager.h"
#include "core/io/power_manager.h"
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
#include "core/ui/global_settings.h"
#include "core/transport_control.h"
#include "core/midi/octave_shift.h"
#include "core/io/button_input_handler.h"
#include "core/io/joystick_input_handler.h"
#include "plugins/input/chord_mapping_input.h"
#include "plugins/input/piano_input.h"
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
PowerManager power_mgr;

// Octave shift system
OctaveShift octave_shift;

// Global settings
GlobalSettings global_settings;

// Transport control
TransportControl transport_control;

// Track system
Track main_track;
ChordMappingInput* chord_plugin_ptr = nullptr;  // Keep reference for UI
PianoInput* piano_plugin_ptr = nullptr;  // Keep reference for UI

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
    
    // 3) Initialize display FIRST to show splash screen immediately
    // This provides immediate visual feedback and better UX
    io_manager.SetHardware(&hw);  // Set hw pointer so io_manager works properly
    io_manager.GetDisplay()->Init(&hw);
    DisplayManager* display = io_manager.GetDisplay();
    SplashScreen* splash_ptr = nullptr;
    if (display && display->IsHealthy()) {
        ExternalLog::PrintLine("Display: Initialized OK");
        hw.DelayMs(200);  // Give display time to stabilize
        
        // Initialize and show splash screen immediately
        splash_screen.Init(display);
        splash_screen.Render();
        splash_ptr = &splash_screen;
        ExternalLog::PrintLine("Splash screen displayed");
    }
    
    // 4) Delay while splash screen is visible (1000ms = 1 second)
    // This delay allows SD card to stabilize before initialization
    // User sees splash screen immediately, making the delay feel shorter
    if (splash_ptr) {
        // Update splash screen during delay (50ms chunks = 1000ms total)
        for (int i = 0; i < 20; i++) {
            hw.DelayMs(50);
            splash_ptr->Update();
            if (splash_ptr->ShouldShow()) {
                splash_ptr->Render();
            }
        }
    } else {
        // No display, just delay
        hw.DelayMs(1000);
    }
    
    // 5) Configure audio
    hw.SetAudioBlockSize(4);
    ExternalLog::PrintLine("Audio configured");
    
    // 6) Initialize power manager (for power optimization)
    power_mgr.Init(&hw);
    
    // 7) Initialize rest of IO manager (SD card, digital, analog, serial)
    // Display was already initialized above, so we initialize the rest manually
    // This ensures SD card init happens after the delay
    io_manager.GetDigital()->Init(&hw);
    io_manager.GetAnalog()->Init(&hw);
    io_manager.GetSerial()->Init(&hw);
    io_manager.GetStorage()->Init(&hw);  // SD card init happens here (after delay)
    io_manager.SetPowerManager(&power_mgr);
    
    
    // Initialize unified input manager (coordinates buttons, joystick, encoder)
    input_manager.Init(&io_manager);
    
    volume_mgr.SetIO(&io_manager);
    audio_engine.Init(&hw);
    audio_engine.SetVolumeManager(&volume_mgr);
    
    // Audio input configuration - default to line in, processing disabled (power savings)
    audio_engine.SetInputSource(AudioEngine::AudioInputSource::LINE_IN);
    audio_engine.SetAudioInputProcessingEnabled(false);  // Disabled by default for power savings
    
    // Disable mic ADC reads in analog manager (power savings - mic ADC consumes significant power)
    // Mic ADC is only needed when:
    // 1. Audio input processing is enabled AND
    // 2. Input source is set to MICROPHONE
    // Since both are disabled by default, mic ADC is disabled
    io_manager.GetAnalog()->SetMicADCEnabled(false);  // Disabled by default - saves power
    
    // Initialize MIDI handler
    midi_handler.Init(&hw);
    ExternalLog::PrintLine("MIDI handler initialized");
    
    // Initialize global settings
    // (No explicit init needed - constructor handles it)
    ExternalLog::PrintLine("Global settings initialized");
    
    // Initialize transport control
    transport_control.Init(&midi_handler, &global_settings);
    ExternalLog::PrintLine("Transport control initialized");
    
    // Initialize track system
    main_track.Init();
    main_track.SetName("Track 1");
    
    // Set input modes for chord mapping
    input_manager.SetButtonInputMode(InputMode::MIDI_NOTES);
    input_manager.SetJoystickMode(JoystickMode::CHORD_MAPPING);
    
    // Add piano input plugin first (highest priority, default exclusive plugin)
    // This is the default input mode and appears at the top of the menu
    auto piano_plugin = std::make_unique<PianoInput>();
    piano_plugin_ptr = piano_plugin.get();
    piano_plugin->SetInputManager(&input_manager);
    piano_plugin->SetOctaveShift(&octave_shift);
    piano_plugin->SetTrack(&main_track);  // Allow it to check for other active plugins and ensure default activation
    piano_plugin->Init();
    // Add Piano first, then explicitly activate it to deactivate other exclusive plugins
    main_track.AddInputPlugin(std::move(piano_plugin));
    // Piano is already active by default, but ensure it deactivates others
    if (piano_plugin_ptr) {
        main_track.SetInputPluginActive(piano_plugin_ptr, true);
    }
    
    // Create and add chord mapping plugin
    // Add chord mapping plugin (medium priority)
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
    
    ExternalLog::PrintLine("Track system initialized with piano, chord mapping, and drum pad input");
    
    ExternalLog::PrintLine("Managers initialized");
    
    // Initialize UI (splash screen was already shown during SD card delay)
    if (display && display->IsHealthy()) {
            // Initialize UI Manager (centralized UI coordinator)
            ui_manager.Init(display, &input_manager, &io_manager);
            ui_manager.SetTrack(&main_track);
            ui_manager.SetOctaveShift(&octave_shift);  // Provide octave shift for octave UI
            ui_manager.SetContext(nullptr);  // Normal mode
            ui_manager.SetPowerManager(&power_mgr);  // Enable power-aware display refresh
            
            // Set global settings in menu manager
            MenuManager* menu_mgr = ui_manager.GetMenuManager();
            if (menu_mgr) {
                menu_mgr->SetGlobalSettings(&global_settings);
            }
            ExternalLog::PrintLine("UI Manager initialized");
            
            // Initialize main UI (default view)
            main_ui.Init(display, &input_manager);
            main_ui.SetTrack(&main_track);
            main_ui.SetChordPlugin(chord_plugin_ptr);
            main_ui.SetPianoPlugin(piano_plugin_ptr);
            
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
    
    ExternalLog::PrintLine("Audio engine ready");
    
    // 6) Start audio
    hw.StartAudio(AudioCallback);
    ExternalLog::PrintLine("Audio started");
    
    hw.DelayMs(100);
    
    ExternalLog::PrintLine("System initialized OK");
    
    // 7) Main loop - Adaptive timing for power optimization
    while(1) {
        // Update power manager and get recommended loop delay
        uint32_t loop_delay = power_mgr.Update();
        
        io_manager.Update();
        input_manager.Update();       // Update unified input manager (handles all inputs)
        
        // Check for user input activity (buttons, encoder, joystick)
        if (input_manager.IsAnySystemButtonPressed() || 
            input_manager.GetEncoder().GetDelta() != 0.0f ||
            input_manager.GetJoystick().GetDeltaX() != 0.0f ||
            input_manager.GetJoystick().GetDeltaY() != 0.0f) {
            power_mgr.ReportUserInput();
        }
        
        volume_mgr.Update();          // Update volume manager to get latest ADC values
        
        // Update track system (processes input plugins and generates MIDI)
        main_track.Update();
        
        // Get joystick position and route to track
        // Note: Joystick movement activity is already tracked above via GetDeltaX/Y check
        JoystickInputHandler& joystick = input_manager.GetJoystick();
        float joystick_x, joystick_y;
        joystick.GetPosition(&joystick_x, &joystick_y);
        main_track.HandleJoystick(joystick_x, joystick_y);
        
        // Update splash screen
        splash_screen.Update();
        
        // Show splash screen if it should be displayed
        if (splash_screen.ShouldShow()) {
            // Report activity during splash to prevent power optimization from interfering
            power_mgr.ReportActivity();
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
                
                // Track button states for release detection
                static bool prev_input_pressed = false;
                static bool prev_record_pressed = false;
                static bool prev_instrument_pressed = false;
                static bool prev_fx_pressed = false;
                
                bool input_pressed = input_manager.GetButtons().IsSystemButtonPressed(SystemButton::INPUT);
                bool record_pressed = input_manager.GetButtons().IsSystemButtonPressed(SystemButton::RECORD);
                bool instrument_pressed = input_manager.GetButtons().IsSystemButtonPressed(SystemButton::INSTRUMENT);
                bool fx_pressed = input_manager.GetButtons().IsSystemButtonPressed(SystemButton::FX);
                
                // Detect button releases (for menu opening - wait for release to detect combos)
                bool input_released = prev_input_pressed && !input_pressed;
                bool record_released = prev_record_pressed && !record_pressed;
                bool instrument_released = prev_instrument_pressed && !instrument_pressed;
                bool fx_released = prev_fx_pressed && !fx_pressed;
                
                // Button hold detection for menus (500ms threshold)
                // Hold = open menu (menu stays open while held), Tap = MIDI transport (for INPUT and RECORD)
                // Menus close automatically when button is released
                const uint32_t BUTTON_HOLD_THRESHOLD_MS = 250;
                
                static uint32_t input_hold_start_time = 0;
                static uint32_t record_hold_start_time = 0;
                static uint32_t instrument_hold_start_time = 0;
                static uint32_t fx_hold_start_time = 0;
                
                static bool input_menu_open = false;  // Track if menu was opened by hold
                static bool record_menu_open = false;
                static bool instrument_menu_open = false;
                static bool fx_menu_open = false;
                
                #if DEBUG_SCREEN_ENABLED
                bool debug_mode = debug_screen.IsEnabled();
                #else
                bool debug_mode = false;
                #endif
                
                // Only handle button holds/menus if not in debug mode
                if (!debug_mode) {
                    // INPUT button hold detection
                    if (input_pressed) {
                        if (input_hold_start_time == 0) {
                            input_hold_start_time = hw.system.GetNow();
                        } else {
                            uint32_t hold_duration = hw.system.GetNow() - input_hold_start_time;
                            if (hold_duration >= BUTTON_HOLD_THRESHOLD_MS && !input_menu_open) {
                                // INPUT held - open Input Stack menu
                                if (menu_mgr) {
                                    menu_mgr->OpenInputStackMenu();
                                    if (settings_mgr && settings_mgr->GetPlugin()) {
                                        menu_mgr->SetCurrentSettingsPlugin(settings_mgr->GetPlugin());
                                    }
                                }
                                input_menu_open = true;
                            }
                        }
                    } else {
                        // INPUT button released
                        if (input_hold_start_time > 0) {
                            uint32_t press_duration = hw.system.GetNow() - input_hold_start_time;
                            
                            // Close menu if it was open
                            if (input_menu_open && menu_mgr && menu_mgr->GetCurrentMenuType() == MenuManager::MenuType::INPUT_STACK) {
                                menu_mgr->CloseMenu();
                            }
                            
                            // Only trigger transport if it was a tap (released before hold threshold)
                            // Store the duration check result before resetting the timer
                            bool was_tap = (press_duration < BUTTON_HOLD_THRESHOLD_MS);
                            input_hold_start_time = 0;
                            input_menu_open = false;
                            
                            // Trigger play/pause only if it was a tap (not a hold)
                            if (was_tap) {
                                transport_control.HandleCombo(0);  // 0 = Play/Pause
                            }
                        }
                    }
                    
                    // RECORD button hold detection
                    if (record_pressed) {
                        if (record_hold_start_time == 0) {
                            record_hold_start_time = hw.system.GetNow();
                        } else {
                            uint32_t hold_duration = hw.system.GetNow() - record_hold_start_time;
                            if (hold_duration >= BUTTON_HOLD_THRESHOLD_MS && !record_menu_open) {
                                // RECORD held - open Global Settings menu
                                if (menu_mgr) {
                                    menu_mgr->OpenGlobalSettingsMenu();
                                    if (settings_mgr) {
                                        IPluginWithSettings* plugin = menu_mgr->GetCurrentSettingsPlugin();
                                        if (plugin) {
                                            settings_mgr->SetPlugin(plugin);
                                        }
                                    }
                                }
                                record_menu_open = true;
                            }
                        }
                    } else {
                        // RECORD button released
                        if (record_hold_start_time > 0) {
                            uint32_t press_duration = hw.system.GetNow() - record_hold_start_time;
                            
                            // Close menu if it was open
                            if (record_menu_open && menu_mgr && menu_mgr->GetCurrentMenuType() == MenuManager::MenuType::GLOBAL_SETTINGS) {
                                menu_mgr->CloseMenu();
                                if (settings_mgr) {
                                    settings_mgr->SetPlugin(nullptr);
                                }
                            }
                            
                            // Only trigger transport if it was a tap (released before hold threshold)
                            bool was_tap = (press_duration < BUTTON_HOLD_THRESHOLD_MS);
                            record_hold_start_time = 0;
                            record_menu_open = false;
                            
                            // Trigger record only if it was a tap (not a hold)
                            if (was_tap) {
                                transport_control.HandleCombo(1);  // 1 = Record toggle
                            }
                        }
                    }
                    
                    // INSTRUMENT button hold detection
                    if (instrument_pressed) {
                        if (instrument_hold_start_time == 0) {
                            instrument_hold_start_time = hw.system.GetNow();
                        } else {
                            uint32_t hold_duration = hw.system.GetNow() - instrument_hold_start_time;
                            if (hold_duration >= BUTTON_HOLD_THRESHOLD_MS && !instrument_menu_open) {
                                // INSTRUMENT held - open Instrument menu
                                if (menu_mgr) {
                                    menu_mgr->OpenInstrumentMenu();
                                }
                                instrument_menu_open = true;
                            }
                        }
                    } else {
                        // INSTRUMENT button released - close menu if it was open
                        if (instrument_menu_open && menu_mgr && menu_mgr->GetCurrentMenuType() == MenuManager::MenuType::INSTRUMENT) {
                            menu_mgr->CloseMenu();
                        }
                        instrument_hold_start_time = 0;
                        instrument_menu_open = false;
                    }
                    
                    // FX button hold detection
                    if (fx_pressed) {
                        if (fx_hold_start_time == 0) {
                            fx_hold_start_time = hw.system.GetNow();
                        } else {
                            uint32_t hold_duration = hw.system.GetNow() - fx_hold_start_time;
                            if (hold_duration >= BUTTON_HOLD_THRESHOLD_MS && !fx_menu_open) {
                                // FX held - open FX menu
                                if (menu_mgr) {
                                    menu_mgr->OpenFXMenu();
                                }
                                fx_menu_open = true;
                            }
                        }
                    } else {
                        // FX button released - close menu if it was open
                        if (fx_menu_open && menu_mgr && menu_mgr->GetCurrentMenuType() == MenuManager::MenuType::FX) {
                            menu_mgr->CloseMenu();
                        }
                        fx_hold_start_time = 0;
                        fx_menu_open = false;
                    }
                } else {
                    // Debug mode - reset all hold timers and close menus
                    if (input_menu_open && menu_mgr) menu_mgr->CloseMenu();
                    if (record_menu_open && menu_mgr) menu_mgr->CloseMenu();
                    if (instrument_menu_open && menu_mgr) menu_mgr->CloseMenu();
                    if (fx_menu_open && menu_mgr) menu_mgr->CloseMenu();
                    
                    input_hold_start_time = 0;
                    record_hold_start_time = 0;
                    instrument_hold_start_time = 0;
                    fx_hold_start_time = 0;
                    input_menu_open = false;
                    record_menu_open = false;
                    instrument_menu_open = false;
                    fx_menu_open = false;
                }
                
                // Transport triggers are now handled in the button release sections above
                // (moved to be part of the release handling logic)
                
                // Update previous button states
                prev_input_pressed = input_pressed;
                prev_record_pressed = record_pressed;
                prev_instrument_pressed = instrument_pressed;
                prev_fx_pressed = fx_pressed;
                
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
        
        // Print MIDI enabled status once after initialization (reduced logging for power)
        static bool midi_enabled_printed = false;
        static uint32_t init_counter = 0;
        if (!midi_enabled_printed && ++init_counter > 100) {
            #if DEBUG_MODE
            ExternalLog::PrintLine("MIDI Enabled: TRS=%s, USB=%s", 
                        midi_handler.IsTrsInitialized() ? "YES" : "NO",
                        midi_handler.IsUsbInitialized() ? "YES" : "NO");
            #endif
            midi_enabled_printed = true;
        }
        
        // Report audio activity for power management (if note is playing)
        if (audio_engine.IsNoteOn()) {
            power_mgr.ReportAudioActivity();
        }
        
        if (volume_mgr.HasVolumeChanged()) {
            volume_mgr.ClearChangeFlag();
        }
        
        // LED heartbeat - reduced frequency for power savings
        // Only blink when active, slower when idle
        static uint32_t heartbeat = 0;
        uint32_t heartbeat_interval = power_mgr.IsIdle() ? 5000 : 2000;  // 5s when idle, 2s when active
        if (++heartbeat % heartbeat_interval == 0) {
            hw.SetLed(true);   // Turn LED on
            hw.DelayMs(20);    // Keep on for 20ms (reduced from 50ms)
            hw.SetLed(false);  // Turn LED off
        }
        
        // Adaptive timing - use power manager's recommended delay
        // Power savings come from:
        // 1. Reduced ADC sampling (10-100 Hz instead of 1 kHz when idle)
        // 2. Reduced display refresh (1-20 Hz adaptive, power optimized)
        // 3. Disabled mic ADC when not needed (disabled by default)
        // 4. Disabled audio input processing by default (power savings)
        // 5. Adaptive loop frequency (100 Hz-1 kHz based on activity)
        // 
        // Note: Sleep mode (WFI) removed - too finicky and can cause hangs.
        // The above optimizations provide significant power savings without the complexity.
        hw.DelayMs(loop_delay);
    }
} 