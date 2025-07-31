# Project Name
TARGET = OpenChord

# Sources
CPP_SOURCES = src/main.cpp \
              src/core/io.cpp \
              src/core/system.cpp \
              src/core/tracks/track.cpp \
              src/plugins/input/basic_midi_input.cpp

# Include directories
C_INCLUDES = -Iinclude

# Library Locations
LIBDAISY_DIR = lib/libDaisy
DAISYSP_DIR = lib/DaisySP

# Core location, and generic makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile 