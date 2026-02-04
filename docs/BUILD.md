# Building & Uploading to ESP32-S3

## Required Libraries

Install via Arduino Library Manager:
- **RTClib** by Adafruit (for DS3231 RTC)
- **U8g2** (already installed)

Built-in ESP32 libraries (no installation needed):
- WiFi.h
- WebServer.h
- DNSServer.h
- Preferences.h
- mbedtls

## Arduino IDE Setup

1. **Board Selection:**
   - Tools → Board → ESP32 Arduino → ESP32S3 Dev Module

2. **USB Configuration:**
   - USB CDC On Boot: Enabled
   - USB Mode: Hardware CDC and JTAG

3. **Upload:**
   - Connect ESP32-S3 via USB
   - Select correct COM port
   - Click Upload

## File Structure

Arduino IDE automatically compiles all .cpp/.h files in the same folder as .ino:

```
Hardware-TOTP/
├── totpgen.ino          ← Main sketch
├── config.h
├── totp_core.h/cpp
├── buttons.h/cpp
├── display_ui.h/cpp
├── storage.h/cpp
├── rtc_manager.h/cpp
└── web_provision.h/cpp
```

**Important:** All files must be in same directory as totpgen.ino

## First Boot

1. **Enter Setup Mode:**
   - Hold both buttons while powering on
   - OLED shows "Setup Mode"
   - Connect to WiFi: `TOTP-Setup-XXXX`
   - Password: `totpsetup`

2. **Configure:**
   - Browser auto-opens (or go to 192.168.4.1)
   - Add WiFi credentials
   - Add TOTP accounts
   - Device reboots

3. **Normal Operation:**
   - Device connects to WiFi
   - Syncs time via NTP
   - Updates RTC (if present)
   - Ready to generate OTPs

## Optional Hardware

**DS3231 RTC Module** (for offline timekeeping):
- SDA → GPIO 8 (shared with OLED)
- SCL → GPIO 9 (shared with OLED)
- VCC → 3.3V
- GND → GND

Without RTC: Device requires WiFi for time sync.
