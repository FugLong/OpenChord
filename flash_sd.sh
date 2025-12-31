#!/bin/bash
# Flash firmware to SD card for bootloader update
# Usage: ./flash_sd.sh [SD_CARD_MOUNT_POINT]
#
# If SD_CARD_MOUNT_POINT is not provided, script will try to auto-detect
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

# Determine SD card mount point
SD_MOUNT=""

if [ -n "$1" ]; then
    # User provided mount point
    SD_MOUNT="$1"
    if [ ! -d "$SD_MOUNT" ]; then
        print_error "Mount point does not exist: $SD_MOUNT"
        exit 1
    fi
else
    # Auto-detect SD cards (macOS)
    if [[ "$OSTYPE" == "darwin"* ]]; then
        print_info "Searching for mounted SD cards..."
        # Look for common SD card identifiers
        SD_CARDS=($(df -h | grep -E '^/dev/disk[0-9]+s[0-9]+' | awk '{print $9}' | grep -v '/Volumes/Time Machine' | grep -v '/Volumes/Backups' | grep -v '/Volumes/Macintosh HD' || true))
        
        if [ ${#SD_CARDS[@]} -eq 0 ]; then
            print_error "No SD cards found. Please mount an SD card first."
            print_info "Available volumes:"
            ls -1 /Volumes/ 2>/dev/null || true
            print_info ""
            print_info "You can also specify the mount point manually:"
            print_info "  ./flash_sd.sh /Volumes/YOUR_SD_CARD_NAME"
            exit 1
        elif [ ${#SD_CARDS[@]} -eq 1 ]; then
            SD_MOUNT="${SD_CARDS[0]}"
            print_info "Auto-detected SD card: $SD_MOUNT"
        else
            print_info "Multiple drives found:"
            for i in "${!SD_CARDS[@]}"; do
                echo "  $((i+1)). ${SD_CARDS[$i]}"
            done
            read -p "Select SD card number (1-${#SD_CARDS[@]}): " SELECTION
            if [ "$SELECTION" -ge 1 ] && [ "$SELECTION" -le ${#SD_CARDS[@]} ]; then
                SD_MOUNT="${SD_CARDS[$((SELECTION-1))]}"
            else
                print_error "Invalid selection"
                exit 1
            fi
        fi
    else
        # Linux
        print_info "Searching for mounted SD cards..."
        SD_CARDS=($(df -h | grep -E '/media/|/mnt/' | awk '{print $6}' || true))
        
        if [ ${#SD_CARDS[@]} -eq 0 ]; then
            print_error "No SD cards found. Please mount an SD card first."
            print_info "Common mount locations: /media/* /mnt/*"
            exit 1
        elif [ ${#SD_CARDS[@]} -eq 1 ]; then
            SD_MOUNT="${SD_CARDS[0]}"
            print_info "Auto-detected SD card: $SD_MOUNT"
        else
            print_info "Multiple drives found:"
            for i in "${!SD_CARDS[@]}"; do
                echo "  $((i+1)). ${SD_CARDS[$i]}"
            done
            read -p "Select SD card number (1-${#SD_CARDS[@]}): " SELECTION
            if [ "$SELECTION" -ge 1 ] && [ "$SELECTION" -le ${#SD_CARDS[@]} ]; then
                SD_MOUNT="${SD_CARDS[$((SELECTION-1))]}"
            else
                print_error "Invalid selection"
                exit 1
            fi
        fi
    fi
fi

# Verify it's writable
if [ ! -w "$SD_MOUNT" ]; then
    print_error "SD card is not writable: $SD_MOUNT"
    exit 1
fi

TARGET_FILE="$SD_MOUNT/$TARGET_NAME"

# Check if file already exists
if [ -f "$TARGET_FILE" ]; then
    print_warn "File already exists: $TARGET_FILE"
    read -p "Overwrite? (y/N): " CONFIRM
    if [[ ! "$CONFIRM" =~ ^[Yy]$ ]]; then
        print_info "Aborted"
        exit 0
    fi
fi

# Copy binary to SD card
print_info "Copying binary to SD card..."
cp "$BIN_FILE" "$TARGET_FILE"

# Verify copy succeeded
if [ -f "$TARGET_FILE" ]; then
    COPIED_SIZE=$(stat -f%z "$TARGET_FILE" 2>/dev/null || stat -c%s "$TARGET_FILE" 2>/dev/null)
    if [ "$BIN_SIZE" -eq "$COPIED_SIZE" ]; then
        print_info "✓ Successfully copied to: $TARGET_FILE"
        print_info ""
        print_info "Next steps:"
        print_info "  1. Insert SD card into device"
        print_info "  2. Power cycle the device"
        print_info "  3. Bootloader will detect and flash the firmware automatically"
        print_info "  4. LED will indicate flashing progress"
        print_info ""
        print_warn "IMPORTANT: Make sure SD card is formatted as FAT32!"
        print_info ""
        print_warn "NOTE: After flashing, remove $TARGET_NAME from SD card"
        print_info "      (or it will try to flash again on next boot)"
        print_info "      Run: ./clean_sd_bin.sh to remove it automatically"
        print_info ""
        
        # NOTE: Don't clean up .bin files here - the bootloader needs the file to flash!
        # The firmware itself will clean up .bin files after it boots (via CleanupBinFiles)
        # Only clean up Mac temp files before ejecting
        print_info "Cleaning up Mac temp files..."
        CLEANUP_COUNT=0
        
        # Delete bootloader log files (.log and bootloader-related .txt files)
        find "$SD_MOUNT" -maxdepth 1 -type f \( -iname "*.log" -o -iname "*boot*.txt" -o -iname "*flash*.txt" \) -print0 | while IFS= read -r -d '' file; do
            rm -f "$file"
            print_info "  Deleted: $(basename "$file")"
            CLEANUP_COUNT=$((CLEANUP_COUNT + 1))
        done
        
        # Delete Mac temp files (only in root, not recursive to avoid deleting user files)
        # .DS_Store - folder metadata
        if [ -f "$SD_MOUNT/.DS_Store" ]; then
            rm -f "$SD_MOUNT/.DS_Store"
            print_info "  Deleted: .DS_Store"
            CLEANUP_COUNT=$((CLEANUP_COUNT + 1))
        fi
        
        # ._* files - Mac resource forks (hidden files starting with ._)
        find "$SD_MOUNT" -maxdepth 1 -type f -name "._*" -print0 | while IFS= read -r -d '' file; do
            rm -f "$file"
            print_info "  Deleted: $(basename "$file")"
            CLEANUP_COUNT=$((CLEANUP_COUNT + 1))
        done
        
        # Remove Mac metadata directories if they exist (only if empty)
        # .Spotlight-V100 - Spotlight index
        if [ -d "$SD_MOUNT/.Spotlight-V100" ]; then
            rmdir "$SD_MOUNT/.Spotlight-V100" 2>/dev/null && print_info "  Removed: .Spotlight-V100 (empty)" && CLEANUP_COUNT=$((CLEANUP_COUNT + 1)) || true
        fi
        
        # .Trashes - trash folder
        if [ -d "$SD_MOUNT/.Trashes" ]; then
            rmdir "$SD_MOUNT/.Trashes" 2>/dev/null && print_info "  Removed: .Trashes (empty)" && CLEANUP_COUNT=$((CLEANUP_COUNT + 1)) || true
        fi
        
        # .fseventsd - file system events
        if [ -d "$SD_MOUNT/.fseventsd" ]; then
            rmdir "$SD_MOUNT/.fseventsd" 2>/dev/null && print_info "  Removed: .fseventsd (empty)" && CLEANUP_COUNT=$((CLEANUP_COUNT + 1)) || true
        fi
        
        # .VolumeIcon.icns - volume icon
        if [ -f "$SD_MOUNT/.VolumeIcon.icns" ]; then
            rm -f "$SD_MOUNT/.VolumeIcon.icns"
            print_info "  Deleted: .VolumeIcon.icns"
            CLEANUP_COUNT=$((CLEANUP_COUNT + 1))
        fi
        
        if [ "$CLEANUP_COUNT" -gt 0 ]; then
            print_info "✓ Cleaned up $CLEANUP_COUNT file(s)"
        else
            print_info "✓ No cleanup needed (SD card already clean)"
        fi
        print_info ""
        
        # Automatically eject the SD card volume (same one we just copied to)
        if [[ "$OSTYPE" == "darwin"* ]]; then
            print_info "Ejecting SD card volume: $SD_MOUNT"
            # diskutil eject can take the mount point directly - this ejects only this specific volume
            if diskutil eject "$SD_MOUNT" > /dev/null 2>&1; then
                print_info "✓ Successfully ejected SD card"
            else
                print_warn "Failed to eject automatically. Please eject manually."
            fi
        else
            # Linux - unmount instead of eject
            print_info "Unmounting SD card volume: $SD_MOUNT"
            if umount "$SD_MOUNT" 2>/dev/null; then
                print_info "✓ Successfully unmounted SD card"
            else
                print_warn "Failed to unmount automatically. Please unmount manually."
            fi
        fi
    else
        print_error "File size mismatch! Original: $BIN_SIZE, Copied: $COPIED_SIZE"
        exit 1
    fi
else
    print_error "Copy failed!"
    exit 1
fi


