#!/bin/bash
# Flash firmware to USB drive for bootloader update
# Usage: ./flash_usb.sh [USB_MOUNT_POINT]
#
# If USB_MOUNT_POINT is not provided, script will try to auto-detect
# or prompt you to select from available drives

set -e  # Exit on error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_FILE="$SCRIPT_DIR/build/OpenChord.bin"
TARGET_NAME="OpenChord.bin"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if binary exists
if [ ! -f "$BIN_FILE" ]; then
    print_error "Binary file not found: $BIN_FILE"
    print_info "Building firmware first..."
    cd "$SCRIPT_DIR"
    make
    if [ ! -f "$BIN_FILE" ]; then
        print_error "Build failed or binary still not found"
        exit 1
    fi
fi

# Get binary size for verification
BIN_SIZE=$(stat -f%z "$BIN_FILE" 2>/dev/null || stat -c%s "$BIN_FILE" 2>/dev/null)
print_info "Binary file: $BIN_FILE ($BIN_SIZE bytes)"

# Determine USB mount point
USB_MOUNT=""

if [ -n "$1" ]; then
    # User provided mount point
    USB_MOUNT="$1"
    if [ ! -d "$USB_MOUNT" ]; then
        print_error "Mount point does not exist: $USB_MOUNT"
        exit 1
    fi
else
    # Auto-detect USB drives (macOS)
    if [[ "$OSTYPE" == "darwin"* ]]; then
        print_info "Searching for mounted USB drives..."
        USB_DRIVES=($(df -h | grep -E '^/dev/disk[0-9]+s[0-9]+' | awk '{print $9}' | grep -v '/Volumes/Time Machine' | grep -v '/Volumes/Backups' || true))
        
        if [ ${#USB_DRIVES[@]} -eq 0 ]; then
            print_error "No USB drives found. Please mount a USB drive first."
            print_info "Available volumes:"
            ls -1 /Volumes/ 2>/dev/null || true
            exit 1
        elif [ ${#USB_DRIVES[@]} -eq 1 ]; then
            USB_MOUNT="${USB_DRIVES[0]}"
            print_info "Auto-detected USB drive: $USB_MOUNT"
        else
            print_info "Multiple USB drives found:"
            for i in "${!USB_DRIVES[@]}"; do
                echo "  $((i+1)). ${USB_DRIVES[$i]}"
            done
            read -p "Select drive number (1-${#USB_DRIVES[@]}): " SELECTION
            if [ "$SELECTION" -ge 1 ] && [ "$SELECTION" -le ${#USB_DRIVES[@]} ]; then
                USB_MOUNT="${USB_DRIVES[$((SELECTION-1))]}"
            else
                print_error "Invalid selection"
                exit 1
            fi
        fi
    else
        # Linux
        print_info "Searching for mounted USB drives..."
        USB_DRIVES=($(df -h | grep -E '/media/|/mnt/' | awk '{print $6}' || true))
        
        if [ ${#USB_DRIVES[@]} -eq 0 ]; then
            print_error "No USB drives found. Please mount a USB drive first."
            print_info "Common mount locations: /media/* /mnt/*"
            exit 1
        elif [ ${#USB_DRIVES[@]} -eq 1 ]; then
            USB_MOUNT="${USB_DRIVES[0]}"
            print_info "Auto-detected USB drive: $USB_MOUNT"
        else
            print_info "Multiple USB drives found:"
            for i in "${!USB_DRIVES[@]}"; do
                echo "  $((i+1)). ${USB_DRIVES[$i]}"
            done
            read -p "Select drive number (1-${#USB_DRIVES[@]}): " SELECTION
            if [ "$SELECTION" -ge 1 ] && [ "$SELECTION" -le ${#USB_DRIVES[@]} ]; then
                USB_MOUNT="${USB_DRIVES[$((SELECTION-1))]}"
            else
                print_error "Invalid selection"
                exit 1
            fi
        fi
    fi
fi

# Verify it's writable
if [ ! -w "$USB_MOUNT" ]; then
    print_error "USB drive is not writable: $USB_MOUNT"
    exit 1
fi

TARGET_FILE="$USB_MOUNT/$TARGET_NAME"

# Check if file already exists
if [ -f "$TARGET_FILE" ]; then
    print_warn "File already exists: $TARGET_FILE"
    read -p "Overwrite? (y/N): " CONFIRM
    if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
        print_info "Aborted"
        exit 0
    fi
fi

# Copy binary to USB drive
print_info "Copying binary to USB drive..."
cp "$BIN_FILE" "$TARGET_FILE"

# Verify copy succeeded
if [ -f "$TARGET_FILE" ]; then
    COPIED_SIZE=$(stat -f%z "$TARGET_FILE" 2>/dev/null || stat -c%s "$TARGET_FILE" 2>/dev/null)
    if [ "$BIN_SIZE" -eq "$COPIED_SIZE" ]; then
        print_info "✓ Successfully copied to: $TARGET_FILE"
        print_info ""
        print_info "Next steps:"
        print_info "  1. Safely eject/unmount the USB drive"
        print_info "  2. Connect USB drive to external USB port (pins D29/D30)"
        print_info "  3. Power cycle the device"
        print_info "  4. Bootloader will detect and flash the firmware automatically"
        print_info "  5. LED will indicate flashing progress"
    else
        print_error "File size mismatch! Original: $BIN_SIZE, Copied: $COPIED_SIZE"
        exit 1
    fi
else
    print_error "Copy failed!"
    exit 1
fi









