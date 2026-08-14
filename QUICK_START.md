# Quick Start Guide - ESP32 Internet Radio

Get your radio running in 5 minutes!

## 🚀 Fast Track Setup

### 1. Install PlatformIO (1 minute)
```bash
pip install platformio
```

### 2. Connect Hardware
- Plug ESP32 to USB
- Verify green LED lights up

### 3. Compile & Upload (2-5 minutes first time)
```bash
cd InternetRadio_ESP32_EPaper
pio run -e esp32-dev -t upload
```

### 4. View Serial Output
```bash
pio device monitor -b 115200
```

You should see boot messages starting with:
```
[MAIN] Initializing display...
[MAIN] Setup complete!
```

### 5. Start Using!
- Use **Encoder** to select stations
- Press **Encoder Click** to play
- Press **Play/Pause** to pause/resume
- Use **Next/Prev** buttons to skip stations
- Rotate **Encoder** to adjust volume in Bluetooth mode

---

## 🔧 GPIO Quick Reference

| Component | GPIO | Purpose |
|-----------|------|---------|
| **Display** |
| CS | 5 | Chip Select |
| CLK | 18 | Clock |
| MOSI | 23 | Data |
| DC | 17 | Data/Command |
| RES | 16 | Reset |
| BUSY | 4 | Busy Signal |
| **Audio** |
| BCK | 26 | Bit Clock |
| WS | 25 | Word Select |
| DIN | 33 | Data In |
| **Controls** |
| Play/Pause | 34 | Button |
| Next | 35 | Button |
| Previous | 39 | Button |
| Encoder CLK | 36 | Rotation |
| Encoder DT | 32 | Rotation |
| Encoder SW | 14 | Click |

---

## 📱 Operation Modes

### **Radio Mode** (Default)
```
┌────────────────────┐
│  BBC Radio 1       │
│                    │
│  ▶ PLAYING         │
│                    │
│  Now playing info  │
│                    │
│  ← Prev Pause Next →
└────────────────────┘
```
- **Encoder UP/DOWN** = Select station
- **Encoder CLICK** = Play selected
- **Play Button** = Play/Pause
- **Next/Prev** = Skip stations

### **Bluetooth Mode**
```
┌────────────────────┐
│ Bluetooth Audio    │
│                    │
│  ⊙ Listening       │
│                    │
│  ESP32-Radio       │
│  Awaiting stream   │
│                    │
│   Receiving Audio
└────────────────────┘
```
- Long press **Play** to enter
- **Encoder UP/DOWN** = Volume
- **Encoder CLICK** = Return to Radio

---

## 🎯 First-Time Issues

### No Display Output
1. Check SPI connections (5, 18, 23, 17, 16, 4)
2. Verify WeAct module is powered
3. Try pressing RST button on ESP32

### No Sound
1. Check I2S pins (26, 25, 33)
2. Verify PCM5102A is powered
3. Check audio output is connected to speakers/headphones
4. Increase volume with encoder

### WiFi Not Connecting
1. Check if nearby WiFi is available
2. ESP32 only supports 2.4GHz (not 5GHz)
3. Try entering credentials manually (in future config option)

### Serial Monitor Not Showing
1. Check USB cable (not all micro-USB cables work for data)
2. Install CH340 or CP2102 driver:
   - Windows: Search "CH340 driver" online
   - macOS: `brew install ch340g-ch34x-usb-driver`
   - Linux: Usually built-in, try `sudo apt install libusb-1.0-0`
3. Verify correct COM port: `platformio device list`

---

## 📊 Default Stations

| # | Name | Type |
|---|------|------|
| 1 | BBC Radio 1 | Pop/Top 40 |
| 2 | SomaFM Groove | Electronic/Downtempo |
| 3 | Radio Paradise | Eclectic Mix |
| 4 | WFUV | Indie/Alternative |
| 5 | Ambient Sleeping | Ambient/Sleep |
| 6 | NPR | News |

**To add your own stations:**
Edit `src/stations.cpp`, find `loadDefaultStations()`, add:
```cpp
addStation("Station Name", "http://stream-url/path");
```

---

## 🔋 Power Consumption

| Mode | Current | Duration on 2000mAh Battery |
|------|---------|---------------------------|
| WiFi Off | 50mA | ~40 hours |
| Radio Playing | 150mA | ~13 hours |
| Bluetooth Connected | 100mA | ~20 hours |
| Idle/Standby | 80mA | ~25 hours |

---

## 📡 Adding Streaming Services

### Shoutcast/Icecast Streams
Simply add the direct M3U URL:
```cpp
addStation("MyStream", "http://host:port/listen.pls");
```

### HTTP Live Streaming (HLS)
May need special handling - currently best with MP3/AAC streams

### Spotify (Not Supported)
Due to DRM restrictions, Spotify streams can't be used directly.
Alternative: Use Bluetooth mode to stream from your phone!

---

## 🎚️ Volume Levels

- 0-20%: Quiet (podcasts, ambient)
- 20-60%: Normal listening
- 60-80%: Loud (music, parties)
- 80-100%: Very loud (beware of distortion on small speakers)

**Software volume only** - also adjust PCM5102A trim pot if available

---

## ⚡ Advanced Tips

### Extend Runtime
- Use larger capacity battery (5000mAh+)
- Disable display updates frequently
- Use Bluetooth mode (less WiFi use)

### Lower Power Consumption
- Set `CORE_DEBUG_LEVEL=0` in platformio.ini
- Reduce display refresh rate
- Use sleep mode (not yet implemented)

### Better Audio Quality
- Use high-bitrate streams (320kbps MP3)
- Ensure PCM5102A has clean power supply
- Keep I2S cables short and shielded

### WiFi Issues
- Position antenna away from walls
- Ensure 2.4GHz WiFi is available
- Try 802.11n standard (not 11b/g only)

---

## 📝 Serial Debug Commands

In Serial Monitor (at 115200 baud), you'll see real-time logs:

```
[AUDIO] Playing: BBC Radio 1
[INPUT] Encoder UP (1)
[AUDIO] Volume: 85%
[DISPLAY] Update complete
[WIFI] Signal: -45 dBm
```

Use these to diagnose issues!

---

## 🆘 Getting Help

1. **Check serial output first**
   ```bash
   pio device monitor -b 115200
   ```

2. **Enable verbose logging**
   Edit platformio.ini:
   ```ini
   build_flags = -DCORE_DEBUG_LEVEL=4
   ```

3. **Reset to defaults**
   - Press and hold encoder + both adjacent buttons
   - Wait for "Factory Reset" message

4. **Check connections**
   - Verify all GPIO pins
   - Check power supply voltage (5V USB)
   - Test with multimeter if available

---

## ✅ Verification Checklist

- [ ] Code compiles without errors
- [ ] Successfully uploads to ESP32  
- [ ] Display shows boot screen
- [ ] Buttons trigger serial messages
- [ ] Encoder messages appear
- [ ] WiFi connects
- [ ] Station list displays
- [ ] Audio plays from selected station
- [ ] Volume control works
- [ ] Bluetooth mode accessible

---

## 🎉 You're Ready!

Your Internet Radio is now operational. Enjoy your music!

**Pro Tip:** Combine with your favorite streaming service's public stream URLs for unlimited stations.

---

For detailed compilation instructions, see **COMPILATION_GUIDE.md**
For full documentation, see **README.md**
