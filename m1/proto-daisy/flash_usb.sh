#!/bin/bash
# Copy the M1 proto binary onto a USB stick for the OG box bootloader.
# That bootloader looks for OpenChord.bin on the EXTERNAL USB (D29/D30).
#
# Usage: ./flash_usb.sh [USB_MOUNT_POINT]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_FILE="$SCRIPT_DIR/build/OpenChordM1.bin"
TARGET_NAME="OpenChord.bin"

if [ ! -f "$BIN_FILE" ]; then
    echo "Missing $BIN_FILE — run make in m1/proto-daisy first."
    exit 1
fi

USB_MOUNT="${1:-}"
if [ -z "$USB_MOUNT" ]; then
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "Mounted volumes:"
        ls -1 /Volumes/ 2>/dev/null || true
        echo "Pass the mount as the first argument, e.g. ./flash_usb.sh /Volumes/NO NAME"
        exit 1
    fi
    echo "Pass the USB mount point as the first argument."
    exit 1
fi

if [ ! -d "$USB_MOUNT" ]; then
    echo "Mount does not exist: $USB_MOUNT"
    exit 1
fi

cp "$BIN_FILE" "$USB_MOUNT/$TARGET_NAME"
echo "Copied $(wc -c < "$BIN_FILE") bytes to $USB_MOUNT/$TARGET_NAME"
echo "Eject the stick, plug it into the box USB-C (pins 36-37), power cycle."
