#pragma once

#include "daisy_seed.h"
#include <cstdint>

// IOManager is at global scope
class IOManager;

namespace OpenChord {

// Forward declarations
class InputManager;
class VolumeManager;
class AudioEngine;
class OpenChordMidiHandler;
class PowerManager;
class GlobalSettings;
class TrackSettings;
class TransportControl;
class OpenChordSystem;
class OctaveShift;
class UIManager;
class MainUI;
class SplashScreen;
class Track;
class ChordMappingInput;
class PianoInput;

#ifdef DEBUG_SCREEN_ENABLED
class DebugScreen;
#endif

/**
 * System Initializer
 * 
 * Handles all system initialization logic, extracting it from main.cpp
 * to keep main() clean and focused on the main loop.
 */
class SystemInitializer {
public:
    struct InitParams {
        // Hardware
        daisy::DaisySeed* hw;
        
        // Managers (must be initialized, but init order handled here)
        IOManager* io_manager;
        InputManager* input_manager;
        VolumeManager* volume_mgr;
        AudioEngine* audio_engine;
        OpenChordMidiHandler* midi_handler;
        PowerManager* power_mgr;
        UIManager* ui_manager;
        OpenChordSystem* system;
        
        // Settings
        GlobalSettings* global_settings;
        TrackSettings* track_settings;
        
        // Controls
        TransportControl* transport_control;
        OctaveShift* octave_shift;
        
        // UI
        SplashScreen* splash_screen;
        MainUI* main_ui;
        
        // Plugin references (set during init, used by UI)
        ChordMappingInput** chord_plugin_ptr;
        PianoInput** piano_plugin_ptr;
        
#ifdef DEBUG_SCREEN_ENABLED
        DebugScreen* debug_screen;
#endif
    };
    
    SystemInitializer();
    ~SystemInitializer();
    
    /**
     * Initialize the entire system
     * Returns true if initialization succeeded, false otherwise
     */
    bool Initialize(const InitParams& params);
    
private:
    // Initialization steps (private helper methods)
    void InitLogging(daisy::DaisySeed* hw);
    bool InitDisplay(IOManager* io_manager, daisy::DaisySeed* hw, SplashScreen* splash_screen);
    void ShowSplashScreen(daisy::DaisySeed* hw, SplashScreen* splash_screen);
    void InitAudio(daisy::DaisySeed* hw);
    void InitIOManagers(IOManager* io_manager, daisy::DaisySeed* hw, PowerManager* power_mgr);
    void InitInputSystem(InputManager* input_manager, IOManager* io_manager);
    void InitAudioSystem(AudioEngine* audio_engine, VolumeManager* volume_mgr, 
                        IOManager* io_manager, daisy::DaisySeed* hw);
    void InitMIDI(OpenChordMidiHandler* midi_handler, daisy::DaisySeed* hw);
    void InitTransportControl(TransportControl* transport_control, 
                             OpenChordMidiHandler* midi_handler, 
                             GlobalSettings* global_settings);
    void InitSystem(OpenChordSystem* system, VolumeManager* volume_mgr, 
                   OctaveShift* octave_shift, daisy::DaisySeed* hw);
    void SetupDefaultTrack(OpenChordSystem* system, InputManager* input_manager,
                          OctaveShift* octave_shift, daisy::DaisySeed* hw,
                          ChordMappingInput** chord_plugin_ptr,
                          PianoInput** piano_plugin_ptr);
    void AddAllFXPluginsToTrack(Track* track, daisy::DaisySeed* hw);
    void InitUI(UIManager* ui_manager, MainUI* main_ui, OpenChordSystem* system,
               InputManager* input_manager, IOManager* io_manager,
               GlobalSettings* global_settings, TrackSettings* track_settings,
               PowerManager* power_mgr, OctaveShift* octave_shift,
               ChordMappingInput* chord_plugin_ptr, PianoInput* piano_plugin_ptr,
               daisy::DaisySeed* hw);
};

} // namespace OpenChord

