#!/bin/bash

# Native Flash Script (without Docker device mapping)
# This uses esptool directly from the build output

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}🔥 Firefly Pixie Native Flash${NC}"
echo "=============================="

# Auto-detect device
DEVICE_PATTERNS=("/dev/tty.usbmodem*" "/dev/tty.usbserial-*" "/dev/tty.SLAB_USBtoUART*")
DEVICE=""

for pattern in "${DEVICE_PATTERNS[@]}"; do
    devices=$(ls $pattern 2>/dev/null || true)
    if [ ! -z "$devices" ]; then
        DEVICE=$(echo $devices | head -n1)
        break
    fi
done

if [ -z "$DEVICE" ]; then
    echo -e "${RED}❌ No ESP32 device found${NC}"
    echo "Available devices:"
    ls /dev/tty.* 2>/dev/null | head -10
    exit 1
fi

echo -e "${GREEN}✅ Found device: $DEVICE${NC}"

# Build first
echo ""
echo "🔨 Building firmware..."
docker run --rm -v $PWD:/project -w /project -e HOME=/tmp espressif/idf idf.py build

if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Build failed${NC}"
    exit 1
fi

# Check if esptool is available locally
ESPTOOL_CMD=""
if command -v esptool &> /dev/null; then
    ESPTOOL_CMD="esptool"
elif python -m esptool --help &> /dev/null; then
    ESPTOOL_CMD="python -m esptool"
elif python3 -m esptool --help &> /dev/null; then
    ESPTOOL_CMD="python3 -m esptool"
else
    echo ""
    echo "⚠️  esptool not found. Installing via pip..."
    echo ""
    echo "Run this command to install esptool:"
    echo "  pip install esptool"
    echo ""
    echo "Or use Homebrew:"
    echo "  brew install esptool"
    echo ""
    echo "Then run this script again."
    exit 1
fi

echo "🔧 Using esptool: $ESPTOOL_CMD"

echo ""
echo "⚡ Flashing with native esptool..."

# Flash using native esptool
$ESPTOOL_CMD --chip esp32c3 -p $DEVICE -b 460800 --before default_reset --after hard_reset write_flash \
    --flash_mode dio --flash_freq 80m --flash_size 16MB \
    0x0 build/bootloader/bootloader.bin \
    0x8000 build/partition_table/partition-table.bin \
    0x10000 build/pixie.bin

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}🎉 Flash successful!${NC}"
    echo ""
    echo "🔧 To monitor serial output:"
    echo "  screen $DEVICE 115200"
    echo "  # Press Ctrl+A then K to exit screen"
else
    echo -e "${RED}❌ Flash failed${NC}"
    exit 1
fi