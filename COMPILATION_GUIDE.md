# Detailed Compilation Guide

This guide will walk you through compiling and uploading the ESP32 Internet Radio firmware.

## Reproducible V1 build environment

The V1 baseline is pinned in `platformio.ini`. The successful reference build
used the following exact environment; do not upgrade individual components
without a separate clean build and hardware regression test.

| Component | Pinned version |
|---|---|
| PlatformIO Core | 6.1.19 |
| Espressif32 platform | 7.0.1 |
| Arduino-ESP32 framework | 3.20017.241212+sha.dcc1105b (Arduino core 2.0.17) |
| Xtensa GCC toolchain | 8.4.0, crosstool-NG esp-2021r2-patch5 |
| GxEPD2 | 1.6.9 |
| Adafruit GFX | 1.12.6 |
| Adafruit BusIO | 1.17.4 |
| Bounce2 | 2.72 |
| ArduinoJson | 7.4.3 |
| ESP32-A2DP | commit `3245602afc494f9e62160a0cfb2af864af45a37f` |
| ESP8266Audio | commit `058e131b26e459b9aadcb589a50f07877f1a09fd` |

The ESP8266Audio AAC+/SBR compatibility patch remains applied by
`tools/patch_esp8266audio_aac.py` before each build. This is intentional and
must not be removed or replaced with an unverified library upgrade.

Use the exact environment and target:

```bash
pio run -e esp32-dev -t clean
pio run -e esp32-dev
```

## System Requirements

- **Operating System**: Windows 10+, macOS 10.13+, Linux (Ubuntu 18.04+)
- **Python**: 3.7 or higher
- **USB Cable**: Micro-USB to USB-A (for ESP32 DevKit V1)
- **Disk Space**: ~500 MB for toolchain and dependencies

## Step 1: Install PlatformIO

### Option A: Using Pip (Recommended)
```bash
pip install --user platformio
```

### Option B: Using Pipx (Isolated)
```bash
# Install pipx first
curl https://pypa.github.io/get-pipx.py | python3
# Then install platformio
pipx install platformio
```

### Verify Installation
```bash
platformio --version
# Output: PlatformIO Core 6.1.x
```

## Step 2: Prepare Your Project

1. **Extract/Clone the project**
```bash
cd ~/projects/InternetRadio_ESP32_EPaper
```

2. **Check project structure**
```bash
ls -la
# Should show: platformio.ini, README.md, src/
```

## Step 3: Connect Your Hardware

1. **Connect ESP32 to Computer**
   - Plug in USB cable from computer to ESP32 DevKit V1 micro-USB port
   - LED on ESP32 should light up

2. **Identify Serial Port**
   
   **Windows:**
   ```powershell
   # Open Device Manager, look for "CH340" or "CP2102" under "Ports (COM & LPT)"
   # Note the COM port (e.g., COM3)
   ```
   
   **macOS/Linux:**
   ```bash
   ls /dev/tty.*
   # Look for /dev/ttyUSB0, /dev/ttyUSB1, or /dev/tty.SLAB_USBtoUART
   ```

3. **Update platformio.ini with your port** (optional - PIO auto-detects)
```ini
upload_port = /dev/ttyUSB0  ; Linux
; upload_port = COM3        ; Windows
; upload_port = /dev/tty.SLAB_USBtoUART  ; macOS
```

## Step 4: Compile the Project

### Full Build
```bash
pio run -e esp32-dev
```

### What You'll See
```
Processing esp32-dev (platform: espressif32; board: esp32doit-devkit-v1; framework: arduino)
Platform Manager: Installing espressif32@6.x.x
Downloading toolchain...
Building project...
[===] COMPLETE [100%]
```

**First build** takes 5-10 minutes (downloads ~500MB)
**Subsequent builds** take 30-60 seconds

### Common Build Errors

#### 1. "Cannot find PlatformIO"
```bash
# Restart terminal or:
export PATH="$PATH:~/.local/bin"  # Linux/macOS
# Windows: Restart PowerShell
```

#### 2. "espressif32 platform not found"
```bash
# Install platform explicitly
pio platform install espressif32
```

#### 3. "Library X not found"
```bash
# Reinstall dependencies
rm -rf .pio/
pio run -e esp32-dev
```

#### 4. Memory Error: "program won't fit"
```
Error: program size (XXX bytes) exceeds maximum (1835008 bytes) for board
```
Solution: The project is too large for default memory. Options:
- Reduce debug symbols: Change `CORE_DEBUG_LEVEL` to 0 in platformio.ini
- Disable unused features in code

## Step 5: Upload to ESP32

### Automatic Upload (Recommended)
```bash
pio run -e esp32-dev -t upload
```

### Manual Upload (If auto-detect fails)
```bash
pio run -e esp32-dev -t upload --upload-port /dev/ttyUSB0
```

### What You'll See During Upload
```
Uploading .pio/build/esp32-dev/firmware.bin
esptool.py v3.3
Serial port /dev/ttyUSB0
Connecting.... (press reset if stuck)
Writing at 0x00010000...
...
Leaving... Hard resetting via RTS pin...
Upload complete! ✓
```

### Upload Troubleshooting

**Stuck at "Connecting":**
- Press BOOT button on ESP32
- Or press RST (reset) button
- Verify USB cable is working

**Device not found:**
```bash
# List available ports
platformio device list
```

**COM Port in use (Windows):**
```powershell
# Close Serial Monitor or other software
# Try different USB port
# Update USB driver for CH340/CP2102
```

**Permission denied (Linux):**
```bash
# Add current user to dialout group
sudo usermod -a -G dialout $USER
# Log out and log back in
```

## Step 6: Monitor Serial Output

### View Debug Messages
```bash
pio device monitor -b 115200
```

### First Boot Output
```
[MAIN] Initializing display...
[DISPLAY] E-paper initialized
[MAIN] Initializing input controls...
[INPUT] Initialized: Buttons and Encoder
[MAIN] Initializing audio system...
[AUDIO] I2S initialized
[MAIN] Loading radio stations...
[STATION] Added: BBC Radio 1
...
[MAIN] Setup complete!
```

### Exit Monitor
Press `Ctrl+C` to exit

## Step 7: Verify Hardware

### Test Display
- Should show "ESP32 Radio" boot message
- Then show "Connecting WiFi..."
- Then show station list

### Test Buttons
- Press any button
- Look for message in serial monitor: `[INPUT] Play/Pause pressed`

### Test Encoder
- Rotate encoder
- Should see: `[INPUT] Encoder UP (1)` or `[INPUT] Encoder DOWN (-1)`

### Test Audio
- Play a radio station
- Should appear in serial monitor: `[AUDIO] Playing: BBC Radio 1`
- Connect headphones to PCM5102A DAC output

## Build Variants

### Debug Build (for troubleshooting)
```bash
pio run -e esp32-dev --verbose
```

### Release Build (optimized)
Edit `platformio.ini`:
```ini
build_flags = 
    -DCORE_DEBUG_LEVEL=0
    -O3
```

### Minimal Build (reduced size)
Comment out unused libraries in `src/*.cpp`

## Advanced: Custom Board Variant

If using different ESP32 variant:

### ESP32-S3
```ini
[env:esp32-s3]
board = esp32-s3-devkitc-1
```

### ESP32-C3
```ini
[env:esp32-c3]
board = esp32-c3-devkitm-1
```

## Cleaning Build Artifacts

```bash
# Remove build directory
pio run -e esp32-dev -t clean

# Full clean
rm -rf .pio/ build/
pio run -e esp32-dev
```

## Backing Up Firmware

### Save Current Firmware
```bash
esptool.py --port /dev/ttyUSB0 read_flash 0 0x1000000 esp32_backup.bin
```

### Restore from Backup
```bash
esptool.py --port /dev/ttyUSB0 write_flash 0 esp32_backup.bin
```

## Performance Optimization Tips

1. **Reduce Serial Debug Output**
   - Set `CORE_DEBUG_LEVEL=0` in platformio.ini
   - Saves ~2% CPU, ~10KB RAM

2. **Optimize Libraries**
   - Remove unused library includes
   - Use `-ffunction-sections -fdata-sections`

3. **Memory Usage**
   - Monitor with: `platformio run -e esp32-dev --verbose`
   - Look for "Memory Usage" section

## Uploading Via Web UI (Optional)

PlatformIO can create web flasher:
```bash
pio run -e esp32-dev -t upload --upload-port WEB_ADDRESS
```

## Quick Reference Commands

| Command | Purpose |
|---------|---------|
| `pio run -e esp32-dev` | Compile only |
| `pio run -e esp32-dev -t upload` | Compile & upload |
| `pio device monitor` | Serial monitor |
| `platformio device list` | List COM ports |
| `pio pkg install` | Download libraries |
| `pio run -e esp32-dev -t clean` | Clean build |
| `pio test -e esp32-dev` | Run unit tests |

## Support & Help

### Get Detailed Build Info
```bash
pio run -e esp32-dev -v
```

### Check Board Configuration
```bash
pio boards esp32doit-devkit-v1
```

### View Library Info
```bash
pio pkg list
```

### PlatformIO Documentation
- https://docs.platformio.org/
- https://docs.platformio.org/en/latest/platforms/espressif32.html

### ESP32 Resources
- Official: https://espressif.com/en/products/socs/esp32
- Arduino Core: https://github.com/espressif/arduino-esp32

---

**Compilation successful? Next step: Configure your WiFi and add stations!**
