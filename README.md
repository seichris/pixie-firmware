# 🔥 Firefly Pixie: Firmware

**Ethereum wallet and retro gaming device powered by ESP32-C3**

The Firefly Pixie is a compact handheld device featuring an Ethereum wallet, classic games, and secure hardware capabilities. This firmware provides a complete user experience with intuitive controls and a beautiful 240x240 IPS display.

## 🎮 Features

### Ethereum Wallet
- **Generate HD Wallets**: Create secure Ethereum addresses on-device
- **Address Display**: View checksummed addresses with QR code placeholder
- **Hardware Security**: Private keys never leave the device

### Classic Games
- **🐍 Snake**: Classic snake game with scoring and right-side orientation
- **🧩 Tetris**: Full Tetris implementation with line clearing and level progression  
- **🏓 Pong**: Player vs AI paddle game
- **👾 Le Space**: Original Space Invaders clone

### Additional Features
- **📱 Device Info**: Hardware specifications and attestation
- **🖼️ GIF Viewer**: Display animated GIFs
- **📋 Scrolling Menu**: Circular navigation with separator

## 🎯 Controls

The Firefly Pixie hardware has 4 fully functional physical buttons:

| Physical Button | Label | Key Enum | Hex Value | Function |
|----------------|-------|----------|-----------|----------|
| **Button 1** (top) | 1 | `KeyCancel` | `0x0020` | Primary Action (shoot, rotate, generate) |
| **Button 2** | 2 | `KeyOk` | `0x0010` | Select/Confirm (menu navigation) |
| **Button 3** | 3 | `KeyNorth` | `0x0001` | Up Movement (ship up, menu up) |
| **Button 4** (bottom) | 4 | `KeySouth` | `0x0004` | Down Movement (ship down, menu down) |

**Standardized Controls (90° Counter-Clockwise Orientation):**
- **Button 1**: Primary action (shoot, rotate, generate, boost)
- **Button 2**: Pause/Exit (short press=pause, hold 1s=exit)  
- **Button 3**: Up/Right movement (in rotated 90° space)
- **Button 4**: Down/Left movement (in rotated 90° space)

**Game-Specific Functions:**
- **🚀 Le Space**: Button 1=Shoot, Button 3/4=Ship up/down, Button 2=Reset (hold)
- **🐍 Snake**: Button 1=Rotate direction, Button 2=Pause/Exit, Button 3/4=Smart movement
- **🧩 Tetris**: Button 1=Rotate piece, Button 2=Pause/Exit, Button 3/4=Move right/left
- **🏓 Pong**: Button 1=Speed boost, Button 2=Pause/Exit, Button 3/4=Paddle right/left
- **💰 Wallet**: Button 1=Generate address, Button 2=Exit, Button 3=Toggle QR
- **📱 Menu**: Button 2=Select, Button 3/4=Navigate up/down

## 🔧 Quick Start

### Prerequisites
- [Docker](https://docs.docker.com/engine/install/) installed
- USB-C cable
- esptool installed: `pip install esptool` or `brew install esptool`

### Build and Flash

```bash
# 1. Clone with submodules
git clone --recurse-submodules https://github.com/firefly/pixie-firmware.git
cd pixie-firmware

# 2. Find your device
./find-device.sh

# 3. Flash firmware (recommended method)
./flash-native.sh
```

### Alternative Methods

```bash
# Build only
./build.sh

# Docker-based flashing
./flash.sh /dev/tty.your_device

# Manual build with Docker
docker run --rm -v $PWD:/project -w /project espressif/idf idf.py build
```

## 📁 Scripts Overview

| Script | Description |
|--------|-------------|
| `flash-native.sh` | **Recommended** - Auto-detects device, builds firmware, flashes with native esptool |
| `build.sh` | Builds firmware using Docker, shows output size |
| `find-device.sh` | Scans for ESP32 devices on common ports |
| `flash.sh` | Docker-based flashing with device path parameter |
| `generate.sh` | Development tool - converts PNG assets to C headers |

## 🔍 Monitoring Output

```bash
# Method 1: Built-in screen command
screen /dev/tty.your_device 115200
# Exit: Ctrl+A then K

# Method 2: Docker monitor
docker run --device=/dev/tty.your_device:/dev/tty.your_device --rm -v $PWD:/project -w /project espressif/idf idf.py -p /dev/tty.your_device monitor
```

## 📱 Hardware Specifications

- **Processor**: ESP32-C3 (32-bit RISC-V @ 160MHz)
- **Memory**: 400KB RAM, 16MB Flash, 4KB eFuse
- **Display**: 240×240px IPS 1.3" (16-bit color)
- **Input**: 4 tactile buttons (KeyCancel, KeyOk, KeyNorth, KeySouth)
- **Output**: 4x RGB LEDs (WS2812B)
- **Connectivity**: USB-C, Bluetooth Low Energy
- **Form Factor**: Compact handheld design

## 🛠️ Development

### Update Submodules
```bash
# If you cloned without --recurse-submodules
git submodule update --init --recursive

# Pull upstream changes
git pull --recurse-submodules
```

### Troubleshooting

**Build Issues:**
```bash
# Check submodule status
git submodule status

# Update submodules
git submodule update --init --recursive
git pull --recurse-submodules
```

**Flashing Issues:**
- Use `./flash-native.sh` to avoid Docker device mapping issues
- Hold BOOT button while connecting USB for download mode
- Try lower baud rate: change `-b 460800` to `-b 115200`
- Install [CH340 drivers](https://github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver) on macOS if needed

**Device Detection:**
- **macOS**: `/dev/tty.usbmodem*`, `/dev/tty.usbserial-*`
- **Linux**: `/dev/ttyUSB*`, `/dev/ttyACM*`  
- **Windows**: `COM*`

## 📖 Documentation

- **[FLASHING.md](FLASHING.md)**: Detailed flashing guide with multiple methods
- **[BOOTING.md](BOOTING.md)**: Secure bootloader planning document
- **[Device Hardware](https://github.com/firefly/pixie-device)**: Schematics and PCB design
- **[3D Printed Case](https://github.com/firefly/pixie-case)**: Enclosure design files

## 🎯 Game Orientation

All games are oriented for comfortable handheld use:
- **Player controls**: Located on the right side near physical buttons
- **Score/UI**: Positioned on the left side for clear visibility
- **Consistent layout**: Maintains orientation across all games

## 🔐 Security Features

- **Hardware attestation**: Secure device identity verification
- **Private key isolation**: Cryptographic operations stay on-device  
- **Secure random generation**: Hardware RNG for key generation
- **Flash encryption**: Planned feature for production builds

## 🚀 What's New

- ✅ **Universal Control Scheme**: Consistent button mapping across all apps
- ✅ **Game Orientation**: Right-side player positioning for ergonomic play
- ✅ **Circular Menu Navigation**: Smooth scrolling with visual separator
- ✅ **Improved Wallet UX**: Better positioning, bidirectional QR toggle
- ✅ **Stable Performance**: Resolved memory issues and boot crashes
- ✅ **Production Scripts**: Streamlined build and flash workflow

## 📜 License

BSD License - see LICENSE file for details.

---

**Happy coding! 🔥**