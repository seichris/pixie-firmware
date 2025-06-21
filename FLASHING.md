# 🔥 Firefly Pixie Flashing Guide

## Quick Start

The easiest way to flash your Firefly Pixie:

```bash
# 1. Find your device
./find-device.sh

# 2. Flash firmware (recommended)
./flash-native.sh
```

## Flashing Methods

### Method 1: Native esptool (Recommended)

Uses esptool directly, bypasses Docker device mapping issues:

```bash
# Install esptool if needed
pip install esptool
# or
brew install esptool

# Flash
./flash-native.sh
```

### Method 2: Docker with device mapping

```bash
./flash.sh [device_path]
```

### Method 3: Manual Commands

#### Build:
```bash
docker run --rm -v $PWD:/project -w /project -e HOME=/tmp espressif/idf idf.py build
```

#### Flash:
```bash
esptool --chip esp32c3 -p /dev/tty.your_device -b 460800 \
  --before default_reset --after hard_reset write_flash \
  --flash_mode dio --flash_freq 80m --flash_size 16MB \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/pixie.bin
```

## Troubleshooting

### Device Not Found
- **Run:** `./find-device.sh` to scan for devices
- **Check:** USB cable connection
- **Try:** Different USB port
- **macOS:** Install [CH340 drivers](https://github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver) if needed

### Device Disappears During Flash
- **Common issue** with ESP32-C3 and Docker
- **Solution:** Use `./flash-native.sh` instead
- **Alternative:** Unplug/reconnect device and retry

### Permission Denied
```bash
# Make scripts executable
chmod +x *.sh

# Add user to dialout group (Linux)
sudo usermod -a -G dialout $USER
```

### Flash Failed
1. **Hold BOOT button** while connecting USB (enters download mode)
2. **Try lower baud rate:** Change `-b 460800` to `-b 115200`
3. **Reset device:** Press RESET button after connecting

## Monitoring Output

### Method 1: Screen (built-in)
```bash
screen /dev/tty.your_device 115200
# Exit: Ctrl+A then K
```

### Method 2: Docker monitor
```bash
docker run --device=/dev/tty.your_device:/dev/tty.your_device --rm -v $PWD:/project -w /project espressif/idf idf.py -p /dev/tty.your_device monitor
```

### Method 3: minicom
```bash
brew install minicom
minicom -D /dev/tty.your_device -b 115200
```

## Device Identification

Common ESP32-C3 device names:
- **macOS:** `/dev/tty.usbmodem*`, `/dev/tty.usbserial-*`
- **Linux:** `/dev/ttyUSB*`, `/dev/ttyACM*`
- **Windows:** `COM*`

## Scripts Overview

| Script | Purpose |
|--------|---------|
| `flash-native.sh` | **Recommended** - Uses native esptool |
| `flash.sh` | Docker-based flashing |
| `build.sh` | Build-only |
| `find-device.sh` | Device detection |

## Hardware Info

- **Chip:** ESP32-C3 (RISC-V)
- **Flash:** 16MB
- **Display:** 240x240 IPS
- **Buttons:** 4 directional + center
- **Connectivity:** USB-C, BLE

## What's Included

✅ **Ethereum Wallet** - Generate addresses, view on screen  
✅ **Snake Game** - Classic snake with scoring  
✅ **Tetris** - Full implementation with line clearing  
✅ **Pong** - Player vs AI paddle game  
✅ **Original Features** - Space Invaders, GIFs, device info  

## Controls

- **Navigation:** North/South arrows to move cursor
- **Select:** OK button (center)
- **Back:** West button (left arrow)
- **Game Controls:** Directional buttons for movement