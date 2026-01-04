TARGET = OpenChord
CPP_SOURCES = src/main.cpp src/core/midi/midi_hub.cpp src/core/midi/midi_handler.cpp src/core/midi/midi_router.cpp src/core/midi/octave_shift.cpp src/core/audio/volume_manager.cpp src/core/audio/audio_engine.cpp src/core/system_interface.cpp src/core/system_initializer.cpp src/core/button_controller.cpp src/core/io/io_manager.cpp src/core/io/power_manager.cpp src/core/io/digital_manager.cpp src/core/io/button_input_handler.cpp src/core/io/joystick_input_handler.cpp src/core/io/encoder_input_handler.cpp src/core/io/input_manager.cpp src/core/io/analog_manager.cpp src/core/io/serial_manager.cpp src/core/io/display_manager.cpp src/core/io/storage_manager.cpp src/core/ui/debug_screen.cpp src/core/ui/debug_views.cpp src/core/ui/main_ui.cpp src/core/ui/ui_manager.cpp src/core/ui/system_bar.cpp src/core/ui/content_area.cpp src/core/ui/splash_screen.cpp src/core/ui/menu_manager.cpp src/core/ui/settings_manager.cpp src/core/ui/global_settings.cpp src/core/ui/track_settings.cpp src/core/ui/octave_ui.cpp src/core/transport_control.cpp src/core/music/chord_engine.cpp src/core/tracks/track.cpp src/plugins/input/chord_mapping_input.cpp src/plugins/input/piano_input.cpp src/plugins/input/drum_pad_input.cpp src/plugins/input/basic_midi_input.cpp src/plugins/instruments/subtractive_synth.cpp src/plugins/fx/delay_fx.cpp src/plugins/fx/chorus_fx.cpp src/plugins/fx/flanger_fx.cpp src/plugins/fx/reverb_fx.cpp src/plugins/fx/tremolo_fx.cpp src/plugins/fx/overdrive_fx.cpp src/plugins/fx/phaser_fx.cpp src/plugins/fx/bitcrusher_fx.cpp src/plugins/fx/autowah_fx.cpp src/plugins/fx/wavefolder_fx.cpp

# FatFS character conversion functions (required for LFN support)
C_SOURCES = lib/libDaisy/Middlewares/Third_Party/FatFs/src/option/ccsbcs.c

# Set the libDaisy directory for the core Makefile
LIBDAISY_DIR = lib/libDaisy

# Set the system files directory to the core directory
SYSTEM_FILES_DIR = lib/libDaisy/core

# Set the DaisySP directory
DAISYSP_DIR = lib/DaisySP

# Use QSPI flash for code storage (8MB available vs 128KB internal flash)
APP_TYPE = BOOT_QSPI

# Add floating point support for printf (required for GetFloat() to work)
LDFLAGS += -u _printf_float

# Include the libDaisy core Makefile
include lib/libDaisy/core/Makefile 