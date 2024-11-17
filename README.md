Firefly Pixie: Firmware
=======================

This is early prototype firmware to help design and refine the
Firefly SDK.

It currently amounts to little more than a clone of Space Invaders,
but has many of the necessary API to implement a hardware wallet.

- [Device](https://github.com/firefly/pixie-device) Design, Schematics and PCB
- [Case](https://github.com/firefly/pixie-case)


Hardware Specifications
-----------------------

- **Processor:** ESP32-C3 (32-bit RISC-V)
- **Speed:** 160Mhz
- **Memory:** 400kb RAM, 16Mb Flash, 4kb eFuse
- **Inputs:** 4x tactile buttons
- **Outputs:**
  - 240x240px IPS 1.3" display (16-bit color)
  - 4x RGB LED (WS2812B)
- **Conectivity:**
  - USB-C
  - BLE


License
-------

BSD License.


How to flash your modified firmware to the device (on macOS)
----------------------------------------------------------

1. Install the ESP-IDF (Espressif IoT Development Framework) first:

`brew install cmake ninja dfu-util`

2. Create an esp directory in your home folder and clone ESP-IDF:

```
mkdir ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
```

3. Run the installation script:

```
cd ~/esp/esp-idf
./install.sh
```

4. Add ESP-IDF to your path by adding this to your ~/.zshrc (or ~/.bashrc if using bash):

`alias get_idf='. $HOME/esp/esp-idf/export.sh'`

5. Reload your shell config:
`source ~/.zshrc  # or source ~/.bashrc if using bash`

6. Now you can use the get_idf command to set up your ESP-IDF environment before building:

`get_idf`

After this setup is complete, you can return to your pixie-firmware directory and build the firmware:

```
# Navigate to the firmware directory
cd path/to/pixie-firmware

# Set the target
idf.py set-target esp32

# Configure the project (only needed first time)
idf.py menuconfig

# Build the project
idf.py build
```

7. Find Your Device Port, after pluggin in your device:

`ls /dev/cu.*`

Look for something like `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART`

8. Flash the device and monitor

```
# Replace [PORT] with your actual port from step 6
idf.py -p /dev/cu.usbserial-XXXXX flash monitor
```
