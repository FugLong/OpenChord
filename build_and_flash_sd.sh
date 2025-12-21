#!/bin/bash
# Build firmware and copy to SD card in one step
# Usage: ./build_and_flash_sd.sh [SD_CARD_MOUNT_POINT]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Building firmware..."
cd "$SCRIPT_DIR"
make

echo ""
echo "Copying to SD card..."
"$SCRIPT_DIR/flash_sd.sh" "$@"

