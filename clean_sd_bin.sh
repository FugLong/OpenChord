#!/bin/bash
# Remove OpenChord.bin from SD card after flashing
# Usage: ./clean_sd_bin.sh [SD_CARD_MOUNT_POINT]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_NAME="OpenChord.bin"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Determine SD card mount point
SD_MOUNT=""

if [ -n "$1" ]; then
    SD_MOUNT="$1"
    if [ ! -d "$SD_MOUNT" ]; then
        print_error "Mount point does not exist: $SD_MOUNT"
        exit 1
    fi
else
    # Auto-detect (same logic as flash_sd.sh)
    if [[ "$OSTYPE" == "darwin"* ]]; then
        SD_CARDS=($(df -h | grep -E '^/dev/disk[0-9]+s[0-9]+' | awk '{print $9}' | grep -v '/Volumes/Time Machine' | grep -v '/Volumes/Backups' | grep -v '/Volumes/Macintosh HD' || true))
        
        if [ ${#SD_CARDS[@]} -eq 0 ]; then
            print_error "No SD cards found. Please mount an SD card first."
            ls -1 /Volumes/ 2>/dev/null || true
            exit 1
        elif [ ${#SD_CARDS[@]} -eq 1 ]; then
            SD_MOUNT="${SD_CARDS[0]}"
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
        SD_CARDS=($(df -h | grep -E '/media/|/mnt/' | awk '{print $6}' || true))
        if [ ${#SD_CARDS[@]} -eq 0 ]; then
            print_error "No SD cards found."
            exit 1
        elif [ ${#SD_CARDS[@]} -eq 1 ]; then
            SD_MOUNT="${SD_CARDS[0]}"
        else
            print_info "Multiple drives found:"
            for i in "${!SD_CARDS[@]}"; do
                echo "  $((i+1)). ${SD_CARDS[$i]}"
            done
            read -p "Select SD card number: " SELECTION
            SD_MOUNT="${SD_CARDS[$((SELECTION-1))]}"
        fi
    fi
fi

TARGET_FILE="$SD_MOUNT/$TARGET_NAME"

if [ -f "$TARGET_FILE" ]; then
    print_info "Found: $TARGET_FILE"
    rm "$TARGET_FILE"
    print_info "✓ Removed $TARGET_NAME from SD card"
    print_info "Device will now boot normally without trying to re-flash"
else
    print_warn "No $TARGET_NAME file found on SD card"
    print_info "Nothing to clean up"
fi







