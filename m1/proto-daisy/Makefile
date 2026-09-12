TARGET = OpenChordM1

CPP_SOURCES = main.cpp ../engine/chord_engine.cpp

LIBDAISY_DIR = ../../archive/s1-daisy/lib/libDaisy
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core

C_INCLUDES += -I. -I../engine

# This box already has the OG USB-stick bootloader. Link for QSPI like the
# archive firmware so flash_usb.sh / copying OpenChord.bin still works.
APP_TYPE = BOOT_QSPI
CPP_STANDARD = -std=gnu++17
LDFLAGS += -u _printf_float

include $(LIBDAISY_DIR)/core/Makefile
