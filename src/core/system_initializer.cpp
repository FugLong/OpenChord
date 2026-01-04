#include "system_initializer.h"
#include "config.h"
#include "io/io_manager.h"
#include "io/display_manager.h"
#include "io/input_manager.h"
#include "io/power_manager.h"
#include "audio/volume_manager.h"
#include "audio/audio_engine.h"
#include "midi/midi_handler.h"
#include "ui/global_settings.h"
#include "ui/track_settings.h"
#include "transport_control.h"
#include "system_interface.h"
#include "midi/octave_shift.h"
#include "ui/ui_manager.h"
#include "ui/menu_manager.h"
#include "ui/main_ui.h"
#include "ui/splash_screen.h"
#include "tracks/track_interface.h"
#include "../plugins/input/chord_mapping_input.h"
#include "../plugins/input/piano_input.h"
#include "../plugins/input/drum_pad_input.h"
#include "../plugins/input/basic_midi_input.h"
#include "../plugins/instruments/subtractive_synth.h"
#include "../plugins/fx/delay_fx.h"
#include "../plugins/fx/chorus_fx.h"
#include "../plugins/fx/flanger_fx.h"
#include "../plugins/fx/reverb_fx.h"
#include "../plugins/fx/tremolo_fx.h"
#include "../plugins/fx/overdrive_fx.h"
#include "../plugins/fx/phaser_fx.h"
#include "../plugins/fx/bitcrusher_fx.h"
#include "../plugins/fx/autowah_fx.h"
#include "../plugins/fx/wavefolder_fx.h"
#include "io/button_input_handler.h"
#include "io/joystick_input_handler.h"
#ifdef DEBUG_SCREEN_ENABLED
#include "ui/debug_screen.h"
#endif
#include "daisy_seed.h"
#include "hid/logger.h"
#include <cstring>

namespace OpenChord {

// Use external USB logger for serial output (pins 36-37)
using ExternalLog = daisy::Logger<daisy::LOGGER_EXTERNAL>;

SystemInitializer::SystemInitializer() {
}

SystemInitializer::~SystemInitializer() {
}

bool SystemInitializer::Initialize(const InitParams& params) {
    if (!params.hw) return false;
    
    // 1) Initialize hardware (should be done in main before calling Initialize)
    // params.hw->Init();  // Assume done by caller
    
    // 2) Initialize logging
    InitLogging(params.hw);
    
    // 3) Initialize display and show splash screen
    bool display_ok = InitDisplay(params.io_manager, params.hw, params.splash_screen);
    if (display_ok) {
        ShowSplashScreen(params.hw, params.splash_screen);
    }
    
    // 4) Configure audio
    InitAudio(params.hw);
    
    // 5) Initialize power manager
    params.power_mgr->Init(params.hw);
    
    // 6) Initialize IO managers (digital, analog, serial, storage)
    InitIOManagers(params.io_manager, params.hw, params.power_mgr);
    
    // 7) Initialize input system
    InitInputSystem(params.input_manager, params.io_manager);
    
    // 8) Initialize audio system
    InitAudioSystem(params.audio_engine, params.volume_mgr, params.io_manager, params.hw);
    
    // 9) Initialize MIDI
    InitMIDI(params.midi_handler, params.hw);
    
    // 10) Initialize transport control
    InitTransportControl(params.transport_control, params.midi_handler, params.global_settings);
    
    // 11) Initialize system (multi-track manager)
    InitSystem(params.system, params.volume_mgr, params.octave_shift, params.hw);
    
    // 12) Setup default track with plugins
    SetupDefaultTrack(params.system, params.input_manager, params.octave_shift, params.hw,
                     params.chord_plugin_ptr, params.piano_plugin_ptr);
    
    // 13) Add all FX plugins to track 1 only (all bypassed/off by default)
    // Do this after SetupDefaultTrack to ensure audio is fully initialized
    // Plugins are created but NOT initialized until enabled (saves memory)
    // TODO: Add to other tracks when memory allows
    Track* track1 = params.system->GetTrack(0);
    if (track1) {
        AddAllFXPluginsToTrack(track1, params.hw);
    }
    
    // 13) Wire audio engine to system
    params.audio_engine->SetSystem(params.system);
    
    ExternalLog::PrintLine("System initialized with tracks, plugins, instrument, and FX");
    
    // 14) Initialize UI
    DisplayManager* display = params.io_manager->GetDisplay();
    if (display && display->IsHealthy()) {
        InitUI(params.ui_manager, params.main_ui, params.system, params.input_manager,
              params.io_manager, params.global_settings, params.track_settings,
              params.power_mgr, params.octave_shift,
              *params.chord_plugin_ptr, *params.piano_plugin_ptr, params.hw);
        
        // Debug screen initialization is done in main.cpp because it requires
        // global variables for wrapper functions
    } else {
        ExternalLog::PrintLine("Display: Initialization FAILED");
    }
    
    ExternalLog::PrintLine("Audio engine ready");
    
    return true;
}

void SystemInitializer::InitLogging(daisy::DaisySeed* hw) {
    // Start logging for printing over serial (using external USB pins)
    ExternalLog::StartLog(false);  // False to continue immediately
    
    // Debug mode: Give time to connect to serial before starting
#ifdef DEBUG_MODE
    hw->DelayMs(3000);  // 3 second delay to allow serial connection
#endif
    
    ExternalLog::PrintLine("OpenChord firmware booting...");
}

bool SystemInitializer::InitDisplay(IOManager* io_manager, daisy::DaisySeed* hw, SplashScreen* splash_screen) {
    if (!io_manager || !hw) return false;
    
    // Set hw pointer so io_manager works properly
    io_manager->SetHardware(hw);
    io_manager->GetDisplay()->Init(hw);
    DisplayManager* display = io_manager->GetDisplay();
    
    if (display && display->IsHealthy()) {
        ExternalLog::PrintLine("Display: Initialized OK");
        hw->DelayMs(200);  // Give display time to stabilize
        
        // Initialize and show splash screen immediately
        if (splash_screen) {
            splash_screen->Init(display);
            splash_screen->Render();
            ExternalLog::PrintLine("Splash screen displayed");
        }
        return true;
    }
    
    return false;
}

void SystemInitializer::ShowSplashScreen(daisy::DaisySeed* hw, SplashScreen* splash_screen) {
    if (!hw) return;
    
    // Delay while splash screen is visible (1000ms = 1 second)
    // This delay allows SD card to stabilize before initialization
    if (splash_screen) {
        // Update splash screen during delay (50ms chunks = 1000ms total)
        for (int i = 0; i < 20; i++) {
            hw->DelayMs(50);
            splash_screen->Update();
            if (splash_screen->ShouldShow()) {
                splash_screen->Render();
            }
        }
    } else {
        // No display, just delay
        hw->DelayMs(1000);
    }
}

void SystemInitializer::InitAudio(daisy::DaisySeed* hw) {
    if (!hw) return;
    hw->SetAudioBlockSize(4);
    ExternalLog::PrintLine("Audio configured");
}

void SystemInitializer::InitIOManagers(IOManager* io_manager, daisy::DaisySeed* hw, PowerManager* power_mgr) {
    if (!io_manager || !hw) return;
    
    // Initialize rest of IO manager (SD card, digital, analog, serial)
    // Display was already initialized above, so we initialize the rest manually
    // This ensures SD card init happens after the delay
    io_manager->GetDigital()->Init(hw);
    io_manager->GetAnalog()->Init(hw);
    io_manager->GetSerial()->Init(hw);
    io_manager->GetStorage()->Init(hw);  // SD card init happens here (after delay)
    io_manager->SetPowerManager(power_mgr);
}

void SystemInitializer::InitInputSystem(InputManager* input_manager, IOManager* io_manager) {
    if (!input_manager || !io_manager) return;
    
    // Initialize unified input manager (coordinates buttons, joystick, encoder)
    input_manager->Init(io_manager);
}

void SystemInitializer::InitAudioSystem(AudioEngine* audio_engine, VolumeManager* volume_mgr,
                                       IOManager* io_manager, daisy::DaisySeed* hw) {
    if (!audio_engine || !volume_mgr || !io_manager || !hw) return;
    
    volume_mgr->SetIO(io_manager);
    audio_engine->Init(hw);
    audio_engine->SetVolumeManager(volume_mgr);
    
    // Audio input configuration - default to line in, processing disabled (power savings)
    audio_engine->SetInputSource(AudioEngine::AudioInputSource::LINE_IN);
    audio_engine->SetAudioInputProcessingEnabled(false);  // Disabled by default for power savings
    
    // Disable mic ADC reads in analog manager (power savings - mic ADC consumes significant power)
    io_manager->GetAnalog()->SetMicADCEnabled(false);  // Disabled by default - saves power
}

void SystemInitializer::InitMIDI(OpenChordMidiHandler* midi_handler, daisy::DaisySeed* hw) {
    if (!midi_handler || !hw) return;
    
    midi_handler->Init(hw);
    ExternalLog::PrintLine("MIDI handler initialized");
}

void SystemInitializer::InitTransportControl(TransportControl* transport_control,
                                            OpenChordMidiHandler* midi_handler,
                                            GlobalSettings* global_settings) {
    if (!transport_control || !midi_handler || !global_settings) return;
    
    transport_control->Init(midi_handler, global_settings);
    ExternalLog::PrintLine("Transport control initialized");
}

void SystemInitializer::InitSystem(OpenChordSystem* system, VolumeManager* volume_mgr,
                                   OctaveShift* octave_shift, daisy::DaisySeed* hw) {
    if (!system || !hw) return;
    
    system->Init();
    system->SetSampleRate(hw->AudioSampleRate());
    system->SetBufferSize(4);  // Match audio block size
    system->SetVolumeManager(volume_mgr);
    system->SetOctaveShift(octave_shift);
    system->SetActiveTrack(0);  // Start with track 1
    
    ExternalLog::PrintLine("Global settings initialized");
}

void SystemInitializer::SetupDefaultTrack(OpenChordSystem* system, InputManager* input_manager,
                                         OctaveShift* octave_shift, daisy::DaisySeed* hw,
                                         ChordMappingInput** chord_plugin_ptr,
                                         PianoInput** piano_plugin_ptr) {
    if (!system || !input_manager || !hw) return;
    
    // Get first track for setup
    Track* track1 = system->GetTrack(0);
    if (!track1) return;
    
    track1->SetName("Track 1");
    
    // Set input modes for chord mapping
    input_manager->SetButtonInputMode(InputMode::MIDI_NOTES);
    input_manager->SetJoystickMode(JoystickMode::CHORD_MAPPING);
    
    // Add piano input plugin first (highest priority, default exclusive plugin)
    auto piano_plugin = std::make_unique<PianoInput>();
    if (piano_plugin_ptr) {
        *piano_plugin_ptr = piano_plugin.get();
    }
    piano_plugin->SetInputManager(input_manager);
    piano_plugin->SetOctaveShift(octave_shift);
    piano_plugin->SetTrack(track1);
    piano_plugin->Init();
    track1->AddInputPlugin(std::move(piano_plugin));
    if (piano_plugin_ptr && *piano_plugin_ptr) {
        track1->SetInputPluginActive(*piano_plugin_ptr, true);
    }
    
    // Add chord mapping plugin
    auto chord_plugin = std::make_unique<ChordMappingInput>();
    if (chord_plugin_ptr) {
        *chord_plugin_ptr = chord_plugin.get();
    }
    chord_plugin->SetInputManager(input_manager);
    chord_plugin->SetTrack(track1);
    chord_plugin->Init();
    track1->AddInputPlugin(std::move(chord_plugin));
    
    // Add drum pad plugin
    auto drum_pad_plugin = std::make_unique<DrumPadInput>();
    drum_pad_plugin->SetInputManager(input_manager);
    drum_pad_plugin->Init();
    track1->AddInputPlugin(std::move(drum_pad_plugin));
    
    // Add basic MIDI input plugin for external MIDI (USB/TRS input)
    auto basic_midi_plugin = std::make_unique<BasicMidiInput>();
    basic_midi_plugin->Init();
    basic_midi_plugin->SetActive(true);  // Active by default to receive external MIDI
    track1->AddInputPlugin(std::move(basic_midi_plugin));
    
    // Add subtractive synth instrument
    auto synth = std::make_unique<SubtractiveSynth>();
    synth->SetSampleRate(hw->AudioSampleRate());
    synth->Init();
    track1->SetInstrument(std::move(synth));
}

void SystemInitializer::AddAllFXPluginsToTrack(Track* track, daisy::DaisySeed* hw) {
    if (!track || !hw) return;
    
    float sample_rate = hw->AudioSampleRate();
    
    // Add all FX plugins in musically logical signal chain order
    // All bypassed/off by default - don't call Init() until enabled (saves memory)
    // Signal chain order: Distortion -> Filter -> Modulation -> Time-based
    
    // 1. Distortion/Gain (early in chain)
    auto overdrive = std::make_unique<OverdriveFX>();
    overdrive->SetSampleRate(sample_rate);
    overdrive->SetBypass(true);
    track->AddEffect(std::move(overdrive));
    
    auto bitcrusher = std::make_unique<BitcrusherFX>();
    bitcrusher->SetSampleRate(sample_rate);
    bitcrusher->SetBypass(true);
    track->AddEffect(std::move(bitcrusher));
    
    auto wavefolder = std::make_unique<WavefolderFX>();
    wavefolder->SetSampleRate(sample_rate);
    wavefolder->SetBypass(true);
    track->AddEffect(std::move(wavefolder));
    
    // 2. Filter
    auto autowah = std::make_unique<AutowahFX>();
    autowah->SetSampleRate(sample_rate);
    autowah->SetBypass(true);
    track->AddEffect(std::move(autowah));
    
    // 3. Modulation (in order: phaser, flanger, chorus, tremolo)
    auto phaser = std::make_unique<PhaserFX>();
    phaser->SetSampleRate(sample_rate);
    phaser->SetBypass(true);
    track->AddEffect(std::move(phaser));
    
    auto flanger = std::make_unique<FlangerFX>();
    flanger->SetSampleRate(sample_rate);
    flanger->SetBypass(true);
    track->AddEffect(std::move(flanger));
    
    auto chorus = std::make_unique<ChorusFX>();
    chorus->SetSampleRate(sample_rate);
    chorus->SetBypass(true);
    track->AddEffect(std::move(chorus));
    
    auto tremolo = std::make_unique<TremoloFX>();
    tremolo->SetSampleRate(sample_rate);
    tremolo->SetBypass(true);
    track->AddEffect(std::move(tremolo));
    
    // 4. Time-based (delay before reverb)
    auto delay = std::make_unique<DelayFX>();
    delay->SetSampleRate(sample_rate);
    delay->SetBypass(true);
    track->AddEffect(std::move(delay));
    
    auto reverb = std::make_unique<ReverbFX>();
    reverb->SetSampleRate(sample_rate);
    reverb->SetBypass(true);
    track->AddEffect(std::move(reverb));
}

void SystemInitializer::InitUI(UIManager* ui_manager, MainUI* main_ui, OpenChordSystem* system,
                               InputManager* input_manager, IOManager* io_manager,
                               GlobalSettings* global_settings, TrackSettings* track_settings,
                               PowerManager* power_mgr, OctaveShift* octave_shift,
                               ChordMappingInput* chord_plugin_ptr, PianoInput* piano_plugin_ptr,
                               daisy::DaisySeed* hw) {
    if (!ui_manager || !main_ui || !system || !input_manager || !io_manager || !hw) return;
    
    DisplayManager* display = io_manager->GetDisplay();
    if (!display || !display->IsHealthy()) return;
    
    // Initialize UI Manager (centralized UI coordinator)
    ui_manager->Init(display, input_manager, io_manager);
    ui_manager->SetTrack(system->GetTrack(0));  // Set active track
    ui_manager->SetOctaveShift(octave_shift);  // Provide octave shift for octave UI
    ui_manager->SetContext(nullptr);  // Normal mode
    ui_manager->SetPowerManager(power_mgr);  // Enable power-aware display refresh
    
    // Set global settings and track settings in menu manager
    MenuManager* menu_mgr = ui_manager->GetMenuManager();
    if (menu_mgr) {
        menu_mgr->SetGlobalSettings(global_settings);
        menu_mgr->SetTrackSettings(track_settings);
    }
    ExternalLog::PrintLine("UI Manager initialized");
    
    // Initialize main UI (default view)
    main_ui->Init(display, input_manager);
    main_ui->SetTrack(system->GetTrack(0));  // Set active track
    main_ui->SetChordPlugin(chord_plugin_ptr);
    main_ui->SetPianoPlugin(piano_plugin_ptr);
    
    // Note: MainUI renderer registration is done in main.cpp because it requires
    // a lambda that captures global variables (ContentRenderFunc is a function pointer)
    ui_manager->SetContentType(UIManager::ContentType::MAIN_UI);
    
    // Note: Octave UI check callback is set in main.cpp because it requires
    // a lambda that captures global variables
    ExternalLog::PrintLine("Main UI initialized");
}


} // namespace OpenChord

