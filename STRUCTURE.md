# 📁 Firefly Pixie Firmware - File Structure

## 🏗️ Project Organization

```
pixie-firmware/
├── 📄 README.md                 # Main project documentation
├── 📄 FLASHING.md               # Detailed flashing instructions  
├── 📄 BOOTING.md                # Secure bootloader planning
├── 📄 CHANGELOG.md              # Development session changelog
├── 📄 STRUCTURE.md              # This file - project organization
├── 📄 CMakeLists.txt             # Root build configuration
├── 📄 sdkconfig                  # ESP-IDF configuration
├── 📄 partitions.csv             # Flash partition table
├── 📄 dependencies.lock         # Component dependency versions
│
├── 🔧 Scripts/
│   ├── build.sh                 # Docker-based build script
│   ├── flash.sh                 # Docker-based flashing
│   ├── flash-native.sh          # Native esptool flashing (recommended)
│   ├── find-device.sh           # ESP32 device detection
│   └── generate.sh              # Asset generation from PNG files
│
├── 🎨 Assets/                   # Source images for games and UI
│   ├── alien-1.png             # Space Invaders alien sprite
│   ├── alien-2.png             # Space Invaders alien sprite  
│   ├── background.png           # Menu background
│   ├── pixie.png                # Pixie mascot sprite
│   └── [other game sprites]
│
├── 🏗️ Build/                   # Generated build artifacts (ignored)
│   ├── bootloader/
│   ├── partition_table/
│   └── pixie.bin                # Final firmware binary
│
├── 📦 Components/               # ESP-IDF components
│   ├── firefly-display/        # 240x240 IPS display driver
│   │   ├── CMakeLists.txt       # Component build config
│   │   ├── include/firefly-display.h
│   │   ├── src/display.c        # ST7789 display driver
│   │   └── examples/test-app/   # Display test application
│   │
│   ├── firefly-ethers/         # Ethereum cryptography library
│   │   ├── include/             # Crypto headers (secp256k1, keccak, etc.)
│   │   ├── src/                 # Crypto implementations
│   │   └── CMakeLists.txt
│   │
│   └── firefly-scene/          # 2D graphics and animation engine
│       ├── include/firefly-scene.h
│       ├── src/                 # Scene graph, nodes, animation
│       ├── tools/               # Asset generation tools
│       └── examples/test-app/   # Scene test application
│
└── 🎮 Main/                    # Primary application code
    ├── CMakeLists.txt           # Main component build config
    ├── main.c                   # Application entry point
    ├── config.h                 # Hardware pin definitions
    │
    ├── 🎯 Core Systems/
    │   ├── task-io.c/h          # Display, input, LED management
    │   ├── events.c/h           # Event system for inter-task communication
    │   ├── panel.c/h            # UI panel management system
    │   ├── device-info.c/h      # Hardware info and attestation
    │   └── utils.c/h            # Common utilities
    │
    ├── 🎮 Game Panels/
    │   ├── panel-snake.c/h      # Snake game implementation
    │   ├── panel-tetris.c/h     # Tetris game implementation
    │   ├── panel-pong.c/h       # Pong game implementation
    │   ├── panel-space.c/h      # Space Invaders clone
    │   └── panel-gifs.c/h       # GIF animation viewer
    │
    ├── 💰 Wallet System/
    │   ├── panel-wallet.c/h     # Ethereum wallet interface
    │   └── qr-generator.c/h     # Full-screen QR code display
    │
    ├── 🔧 System Panels/
    │   ├── panel-menu.c/h       # Main menu with circular navigation
    │   ├── panel-connect.c/h    # BLE connectivity panel
    │   ├── panel-attest.c/h     # Device attestation panel
    │   └── panel-buttontest.c/h # Hardware button testing
    │
    ├── 🎨 Assets/               # Generated C headers from PNG files
    │   ├── images/              # Game sprites and UI elements
    │   │   ├── image-alien-1.h  # Space Invaders aliens
    │   │   ├── image-background.h # Menu background
    │   │   ├── image-pixie.h    # Pixie mascot
    │   │   └── [other sprites]
    │   └── video-*.h            # GIF animation data
    │
    └── 📡 Connectivity/
        └── task-ble.c/h         # Bluetooth Low Energy implementation
```

## 🎯 Key Components Explained

### Display System (`firefly-display`)
- **ST7789 Driver**: 240x240 IPS display with SPI interface
- **Fragment Rendering**: Efficient line-by-line display updates
- **Custom Render Support**: Direct framebuffer access for QR codes
- **RGB565 Format**: 16-bit color depth for rich visuals

### Graphics Engine (`firefly-scene`)
- **Scene Graph**: Hierarchical node system for UI elements
- **Animation System**: Smooth transitions and effects
- **Font Rendering**: Multiple font sizes for different UI elements
- **Asset Pipeline**: PNG to C header conversion tools

### Crypto Library (`firefly-ethers`)
- **secp256k1**: Elliptic curve cryptography for Ethereum
- **Keccak Hashing**: Ethereum-compatible hash functions
- **Address Generation**: HD wallet address derivation
- **Hardware RNG**: Secure random number generation

### Application Architecture
- **Panel System**: Modular UI components with standardized controls
- **Event-Driven**: Asynchronous communication between components
- **Task-Based**: FreeRTOS tasks for I/O, display, and connectivity
- **Memory Management**: Efficient allocation for constrained environment

## 🎮 Game Organization

### Control Standardization
All games follow consistent button mapping for 90° rotated orientation:
- **Button 1 (Top)**: Primary action (shoot, rotate, generate)
- **Button 2**: Select/Confirm/Exit  
- **Button 3**: Up/Right movement
- **Button 4 (Bottom)**: Down/Left movement

### Game Implementations
- **Snake**: Classic gameplay with right-side scoring
- **Tetris**: Full line-clearing implementation with level progression
- **Pong**: Player vs AI with speed boost controls
- **Le Space**: Original Space Invaders clone with explosion effects

## 🔧 Build System

### Docker-Based Development
- **Consistent Environment**: ESP-IDF 6.0 in containerized build
- **Cross-Platform**: Works on macOS, Linux, Windows
- **Automated Scripts**: One-command build and flash workflow

### Component Dependencies
- **Modular Design**: Each component has isolated dependencies
- **ESP-IDF Integration**: Native ESP-IDF component structure
- **Version Locked**: Dependency versions tracked in `dependencies.lock`

## 📱 Hardware Integration

### ESP32-C3 Specifics
- **RISC-V Architecture**: 32-bit single-core processor
- **Memory Layout**: 400KB RAM, 16MB Flash optimized usage
- **Pin Configuration**: Defined in `config.h` for hardware abstraction
- **Power Management**: Efficient sleep modes for battery operation

### Peripheral Support
- **SPI Display**: High-speed display updates via SPI2
- **GPIO Buttons**: Debounced input with interrupt handling
- **WS2812B LEDs**: RGB LED strip for status indication
- **USB-C**: Serial communication and firmware updates

---

**File Structure Version**: 1.0  
**Last Updated**: June 22, 2025  
**ESP-IDF Version**: 6.0