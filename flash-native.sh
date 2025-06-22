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
echo "   📦 Cleaning previous build..."

# Redirect verbose output to log file, show only important messages
BUILD_LOG="/tmp/pixie-build.log"
echo "" > "$BUILD_LOG"  # Clear log file

{
    docker run --rm -v $PWD:/project -w /project -e HOME=/tmp -e IDF_TARGET=esp32c3 espressif/idf bash -c "idf.py fullclean > /dev/null 2>&1 && idf.py build" 2>&1
} | while IFS= read -r line; do
    # Log everything to file for debugging
    echo "$line" >> "$BUILD_LOG"
    
    # Show only important build progress messages
    if [[ "$line" =~ "Generating" ]] || [[ "$line" =~ "Creating" ]] || [[ "$line" =~ "Linking" ]] || [[ "$line" =~ "Project build complete" ]] || [[ "$line" =~ "To flash" ]]; then
        printf "   %s\n" "$line"
    elif [[ "$line" =~ "Building" ]] && [[ "$line" =~ ".bin" ]]; then
        printf "   %s\n" "$line"
    fi
done

# Check if build succeeded by looking for the binary files
if [ -f "build/pixie.bin" ] && [ -f "build/bootloader/bootloader.bin" ] && [ -f "build/partition_table/partition-table.bin" ]; then
    echo -e "   ${GREEN}✅ Build completed successfully${NC}"
    echo "   📄 Build log saved to: $BUILD_LOG"
else
    echo -e "${RED}❌ Build failed${NC}"
    echo "📄 Check build log for details: $BUILD_LOG"
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

echo -e "   🔧 Using esptool: ${GREEN}$ESPTOOL_CMD${NC}"

echo ""
echo "⚡ Flashing firmware to device..."
echo "   📋 Target: $DEVICE"
echo "   ⚙️  Chip: ESP32-C3"
echo "   🚀 Baud: 460800"

# Flash using native esptool with cleaner output
FLASH_LOG="/tmp/pixie-flash.log"
echo "" > "$FLASH_LOG"  # Clear log file

{
    $ESPTOOL_CMD --chip esp32c3 -p $DEVICE -b 460800 --before default_reset --after hard_reset write_flash \
        --flash_mode dio --flash_freq 80m --flash_size 16MB \
        0x0 build/bootloader/bootloader.bin \
        0x8000 build/partition_table/partition-table.bin \
        0x10000 build/pixie.bin 2>&1
} | while IFS= read -r line; do
    # Log everything for debugging
    echo "$line" >> "$FLASH_LOG"
    
    # Show progress for important flash steps
    if [[ "$line" =~ "Connecting" ]] || [[ "$line" =~ "Chip is" ]] || [[ "$line" =~ "Uploading stub" ]] || [[ "$line" =~ "Configuring flash" ]] || [[ "$line" =~ "Writing at" ]] || [[ "$line" =~ "Hash of data verified" ]] || [[ "$line" =~ "Leaving" ]]; then
        printf "   %s\n" "$line"
    elif [[ "$line" =~ "%" ]]; then
        # Show progress percentages on same line
        printf "\r   📦 %s" "$line"
    fi
done

echo "" # New line after progress

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}🎉 Flash successful!${NC}"
    echo ""
    echo -e "${YELLOW}🔧 To monitor serial output:${NC}"
    echo -e "  ${GREEN}screen $DEVICE 115200${NC}"
    echo "  # Press Ctrl+A then K to exit screen"
    echo ""
    echo "📄 Flash log saved to: $FLASH_LOG"
else
    echo -e "${RED}❌ Flash failed${NC}"
    echo "📄 Check flash log for details: $FLASH_LOG"
    exit 1
fi