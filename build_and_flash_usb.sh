#!/bin/bash
# Build firmware and copy to USB drive in one step
# Usage: ./build_and_flash_usb.sh [USB_MOUNT_POINT]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Building firmware..."
cd "$SCRIPT_DIR"
make

echo ""
echo "Copying to USB drive..."
"$SCRIPT_DIR/flash_usb.sh" "$@"


