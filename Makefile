TARGET = OpenChord
CPP_SOURCES = src/main.cpp src/core/midi/midi_hub.cpp src/core/midi/midi_handler.cpp src/core/audio/volume_manager.cpp src/core/audio/audio_engine.cpp src/core/io/io_manager.cpp src/core/io/digital_manager.cpp src/core/io/analog_manager.cpp src/core/io/serial_manager.cpp src/core/io/display_manager.cpp src/core/io/storage_manager.cpp

# Set the libDaisy directory for the core Makefile
LIBDAISY_DIR = lib/libDaisy

# Set the system files directory to the core directory
SYSTEM_FILES_DIR = lib/libDaisy/core

# Set the DaisySP directory
DAISYSP_DIR = lib/DaisySP

# Add floating point support for printf (required for GetFloat() to work)
LDFLAGS += -u _printf_float

# Include the libDaisy core Makefile
include lib/libDaisy/core/Makefile 