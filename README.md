# ESP32 Internet Radio + Bluetooth Audio Sink with E-paper Display

A feature-rich internet radio streamer with Bluetooth audio sink support, displaying on a WeAct 1.54" E-paper module (200x200px).

## Features

✅ **Internet Radio Streaming** - Play online radio stations with HTTP streaming
✅ **Bluetooth Audio Sink** - Receive audio from Bluetooth devices (A2DP)
✅ **WiFi Management** - Connect to WiFi networks with credential saving
✅ **E-paper Display** - Beautiful 200x200px B&W display with GxEPD2
✅ **Rotary Encoder** - Station selection and volume control with encoder
✅ **Physical Buttons** - Play/Pause, Next, Previous buttons
✅ **Volume Control** - Software-based volume adjustment
✅ **Station Manager** - Pre-configured stations with easy switching

## Hardware Specifications

### Microcontroller
- **ESP32 DevKit V1** - Dual-core Tensilica LX6, 2.4GHz WiFi, Bluetooth Classic & BLE

### Display
- **WeAct Studio 1.54" E-paper Module**
  - Resolution: 200x200 pixels
  - Color: Black & White
  - Controller: SSD1681
  - Interface: SPI

### Audio Output
- **PCM5102A I2S DAC**
  - 24-bit @ 192kHz capable
  - Configured for 16-bit @ 44.1kHz
  - I2S Audio Interface

### Controls
- **3 Pushbuttons**
  - Play/Pause Button (GPIO34)
  - Next Station Button (GPIO35)
  - Previous Station Button (GPIO39)

- **Rotary Encoder with Click**
  - CLK Pin (GPIO36)
  - DT Pin (GPIO32)
  - Click/Push Button (GPIO14)
  - Function: Station selection & Volume control

## GPIO Pinout

```
┌─────────────────────────────────────┐
│         ESP32 DevKit V1             │
├─────────────────────────────────────┤
│  E-PAPER DISPLAY (SPI)              │
│  CS    → GPIO 5                     │
│  CLK   → GPIO 18 (SCK)              │
│  MOSI  → GPIO 23 (SDA)              │
│  RST   → GPIO 16                    │
│  DC    → GPIO 17                    │
│  BUSY  → GPIO 4                     │
├─────────────────────────────────────┤
│  AUDIO OUTPUT (I2S)                 │
│  BCK   → GPIO 26 (Bit Clock)        │
│  WS    → GPIO 25 (Word Select)      │
│  DIN   → GPIO 33 (Data In)          │
├─────────────────────────────────────┤
│  CONTROLS                            │
│  Button Play/Pause  → GPIO 34       │
│  Button Next        → GPIO 35       │
│  Button Prev        → GPIO 39       │
│  Encoder CLK        → GPIO 36       │
│  Encoder DT         → GPIO 32       │
│  Encoder SW (Click) → GPIO 14       │
└─────────────────────────────────────┘
```

## Project Structure

```
InternetRadio_ESP32_EPaper/
├── platformio.ini              # PlatformIO configuration
├── README.md                   # This file
├── COMPILATION_GUIDE.md        # Detailed compilation guide
├── src/
│   ├── main.cpp               # Main application & state machine
│   ├── config.h               # Hardware configuration & GPIO pins
│   ├── display.h/.cpp         # E-paper display management
│   ├── audio.h/.cpp           # Audio streaming & I2S DAC
│   ├── input.h/.cpp           # Button & encoder input handling
│   ├── net_manager.h/.cpp     # WiFi connection management
│   └── stations.h/.cpp        # Radio station manager
└── lib/                        # External libraries (auto-installed)
```

## Dependencies

The project uses the following libraries (automatically installed by PlatformIO):

1. **GxEPD2** (1.4.6+) - E-paper display driver
   - Supports WeAct 1.54" module with SSD1681

2. **Adafruit GFX Library** (1.11.9+) - Graphics primitives

3. **ESP32-A2DP** (1.7.1+) - Bluetooth A2DP Sink
   - Enables Bluetooth audio reception
   - Uses ESP32's built-in Bluetooth hardware

4. **Bounce2** (2.71.0+) - Button debouncing library

5. **ArduinoJson** (6.21.2+) - JSON parsing (for future use)

## Compilation Instructions

### Prerequisites
- PlatformIO CLI or PlatformIO IDE
- Python 3.7+
- Git

### Setup & Compile

1. **Clone/Download the project**
```bash
cd InternetRadio_ESP32_EPaper
```

2. **Install PlatformIO CLI** (if not already installed)
```bash
pipx install platformio
# or
pip install --user platformio
```

3. **Compile the project**
```bash
pio run -e esp32-dev
```

4. **Upload to ESP32**
```bash
pio run -e esp32-dev -t upload --upload-port /dev/ttyUSB0
```
(Replace `/dev/ttyUSB0` with your actual COM port)

5. **Monitor Serial Output**
```bash
pio device monitor -b 115200 -p /dev/ttyUSB0
```

### Using PlatformIO IDE

1. Open PlatformIO IDE
2. Open folder → Select this project folder
3. Click "Upload" button in the bottom taskbar
4. Click "Serial Monitor" to view debug output

## Usage

### Startup Sequence
1. Power on the device
2. Display shows "ESP32 Radio" boot screen
3. Automatically attempts WiFi connection
4. Shows station list on display

### Operating Modes

#### **Station Selection Mode**
- **Encoder UP/DOWN** - Scroll through stations
- **Encoder CLICK** or **Play/Pause Button** - Play selected station
- **Long Press Play** - Switch to Bluetooth mode

#### **Playing Radio Mode**
- **Play/Pause Button** - Pause/Resume playback
- **Next Button** - Skip to next station
- **Prev Button** - Jump to previous station
- **Encoder UP** - Increase volume
- **Encoder DOWN** - Decrease volume
- **Encoder CLICK** - Return to station selection

#### **Bluetooth Mode**
- **Encoder UP/DOWN** - Volume control
- **Encoder CLICK** or **Long Press** - Return to station selection
- Device appears as "ESP32-Radio" in Bluetooth settings
- Automatically receives audio from paired device

## Default Radio Stations

The firmware includes pre-configured stations:
1. BBC Radio 1
2. SomaFM - Groove Salad
3. Radio Paradise
4. WFUV - Curated Radio
5. Ambient Sleeping Pill
6. NPR

You can modify these in `src/stations.cpp` or add/remove stations via configuration.

## Configuration

### WiFi Credentials
- Automatically saved to EEPROM after first successful connection
- Edit `src/config.h` to change max WiFi credentials storage

### Audio Settings
- Sample Rate: 44.1 kHz (configurable in `src/config.h`)
- Bit Depth: 16-bit
- Channels: Stereo (2)
- Default Volume: 80%

### Display Settings
- Rotation: Landscape (can be changed)
- Font: Free Mono Bold 9pt & Free Serif 9pt
- Update Mode: Full & Partial updates

## Building Custom

### Adding New Radio Stations
Edit `src/stations.cpp` and add to `loadDefaultStations()`:
```cpp
addStation("Station Name", "http://stream-url.com/path");
```

### Changing GPIO Pins
Edit `src/config.h` and modify the pin definitions:
```cpp
#define BUTTON_PLAY_PIN     34
#define I2S_BCK_PIN         26
// etc.
```

### Adjusting Audio Parameters
In `src/config.h`:
```cpp
#define SAMPLE_RATE     44100    // Can use 48000, 96000, etc.
#define I2S_CHANNELS    2        // Stereo (2) or Mono (1)
#define I2S_BITS        16       // 16-bit samples
```

## Troubleshooting

### No Sound Output
1. Check PCM5102A DAC connections and I2S pins
2. Verify audio buffer isn't full - check serial output
3. Try increasing volume with encoder

### WiFi Won't Connect
1. Check SSID and password in setup
2. Verify WiFi is broadcasting
3. Check antenna connections

### E-paper Display Not Updating
1. Verify SPI bus connections (CS, CLK, MOSI)
2. Check BUSY and RST pins
3. Try full screen refresh - unplug and restart

### Bluetooth Not Working
1. Device should show as "ESP32-Radio" in Bluetooth discovery
2. Check if dual-stack (WiFi + BT) is using too much power
3. Reset Bluetooth from display menu

### Memory Issues
If getting low memory warnings:
1. Disable unused features in code
2. Reduce audio buffer size (careful - may cause underruns)
3. Use PSRAM if available on your board variant

## Performance Notes

- **WiFi Streaming**: ~96 kbps typical bandwidth
- **Bluetooth A2DP**: 64-320 kbps (codec dependent)
- **Display Refresh**: ~2 seconds full update, <500ms partial
- **CPU Usage**: ~15-20% during playback
- **RAM Usage**: ~180 KB of 320 KB available

## Future Enhancements

- [ ] SD Card support for local file playback
- [ ] MQTT control interface
- [ ] NTP time display
- [ ] Equalizer with hardware filters
- [ ] Recording to SD card
- [ ] OTA (Over-The-Air) firmware updates
- [ ] Web interface for station configuration
- [ ] Preset presets memory

## License

This project is provided as-is for educational and personal use.

## Contributing

Feel free to fork and submit pull requests for improvements!

## Support

For issues and questions, check:
1. Serial monitor output (115200 baud)
2. GPIO connections match pinout diagram
3. Library versions in platformio.ini

---

**Built with ❤️ for ESP32 audio enthusiasts**
