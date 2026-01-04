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
#include "core/midi/midi_interface.h"
// Note: track_interface.h includes plugin_interface.h which includes midi_types.h
// This defines a different MidiEvent than midi_interface.h
// We'll use the track system's MidiEvent (from midi_types.h) via the Track interface
#include "core/tracks/track_interface.h"
#include "core/system_interface.h"
#include "core/ui/main_ui.h"
#include "core/ui/ui_manager.h"
#include "core/ui/splash_screen.h"
#include "core/ui/menu_manager.h"
#include "core/ui/settings_manager.h"
#include "core/ui/global_settings.h"
#include "core/ui/track_settings.h"
#include "core/transport_control.h"
#include "core/midi/octave_shift.h"
#include "core/io/button_input_handler.h"
#include "core/io/joystick_input_handler.h"
#include "core/system_initializer.h"
#include "core/button_controller.h"
#include "core/midi_router.h"
#include "plugins/input/chord_mapping_input.h"
#include "plugins/input/piano_input.h"
#include "plugins/input/drum_pad_input.h"
#include "plugins/input/basic_midi_input.h"
#include "plugins/instruments/subtractive_synth.h"
#include "plugins/fx/delay_fx.h"
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
TrackSettings track_settings;

// Transport control
TransportControl transport_control;

// System (multi-track manager)
OpenChordSystem openchord_system;
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
    
    // 2) Initialize system using SystemInitializer
    SystemInitializer initializer;
    SystemInitializer::InitParams params;
    params.hw = &hw;
    params.io_manager = &io_manager;
    params.input_manager = &input_manager;
    params.volume_mgr = &volume_mgr;
    params.audio_engine = &audio_engine;
    params.midi_handler = &midi_handler;
    params.power_mgr = &power_mgr;
    params.ui_manager = &ui_manager;
    params.system = &openchord_system;
    params.global_settings = &global_settings;
    params.track_settings = &track_settings;
    params.transport_control = &transport_control;
    params.octave_shift = &octave_shift;
    params.splash_screen = &splash_screen;
    params.main_ui = &main_ui;
    params.chord_plugin_ptr = &chord_plugin_ptr;
    params.piano_plugin_ptr = &piano_plugin_ptr;
#if DEBUG_SCREEN_ENABLED
    params.debug_screen = &debug_screen;
#endif
    
    if (!initializer.Initialize(params)) {
        ExternalLog::PrintLine("System initialization failed!");
        while(1) {
            hw.DelayMs(1000);
        }
    }
    
    // Setup UI renderers (requires lambdas that capture global variables)
    DisplayManager* display = io_manager.GetDisplay();
    if (display && display->IsHealthy()) {
        // Register MainUI renderer with UI Manager
        ui_manager.SetMainUIRenderer([](DisplayManager* disp) {
            main_ui.Render(disp);
        });
        
        // Set octave UI check callback for chord plugin
        if (chord_plugin_ptr) {
            chord_plugin_ptr->SetOctaveUICheckCallback([]() -> bool {
                return ui_manager.IsOctaveUIActive();
            });
        }
        
#if DEBUG_SCREEN_ENABLED
        // Initialize debug screen (wrapper functions use global instances)
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
        ExternalLog::PrintLine("Debug screen initialized");
#endif
    }
    
    // 3) Start audio
    hw.StartAudio(AudioCallback);
    ExternalLog::PrintLine("Audio started");
    
    hw.DelayMs(100);
    ExternalLog::PrintLine("System initialized OK");
    
    // 4) Initialize controllers
    ButtonController button_controller;
    MenuManager* menu_mgr = ui_manager.GetMenuManager();
    SettingsManager* settings_mgr = ui_manager.GetSettingsManager();
    button_controller.Init(&hw, &input_manager, menu_mgr, settings_mgr, 
                          &transport_control, &ui_manager, &io_manager);
#if DEBUG_SCREEN_ENABLED
    button_controller.SetDebugScreen(&debug_screen);
#endif
    
    MidiRouter midi_router;
    midi_router.Init(&openchord_system, &midi_handler, &octave_shift);
    
    // 5) Main loop - Adaptive timing for power optimization
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
        
        // Update system (updates all tracks)
        openchord_system.Update();
        
        // Get joystick position and route to active track
        JoystickInputHandler& joystick = input_manager.GetJoystick();
        float joystick_x, joystick_y;
        joystick.GetPosition(&joystick_x, &joystick_y);
        openchord_system.HandleJoystick(joystick_x, joystick_y);
        
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
                // Handle button controls (menu opening/closing, transport controls)
                button_controller.Update();
                
                // Handle menu/settings navigation (centralized in MenuManager)
                if (menu_mgr && menu_mgr->IsOpen()) {
                    uint32_t current_time = hw.system.GetNow();
                    menu_mgr->UpdateMenuInput(settings_mgr, &io_manager, current_time);
                    
                    // Update context in system bar
                    ui_manager.SetContext(menu_mgr->GetContextName());
                } else {
                    // Normal mode - handle octave UI (stick click)
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
                    
                    ui_manager.SetContentType(UIManager::ContentType::MAIN_UI);
                    ui_manager.SetContext(nullptr);  // Normal mode - show track name
                }
            }
            
            // Update UI Manager (handles all state updates and rendering)
            // UI Manager owns the display lifecycle and coordinates all rendering
            ui_manager.Update();
        }
        
        // Process incoming MIDI events at 1kHz for responsive timing
        midi_handler.ProcessMidi();
        
        // Route MIDI using MidiRouter
        midi_router.RouteMIDI();
        
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
        
        // Report audio activity for power management
        // (AudioEngine.IsNoteOn() always returns false now - TODO: query system/tracks for active notes)
        // For now, we assume audio activity when system is running
        
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
        hw.DelayMs(loop_delay);
    }
}
