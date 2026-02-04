# Hardware TOTP Generator

A physical 2FA device built on ESP32-S3 that generates time-based one-time passwords (TOTP). No phone needed - just press a button and it types your OTP directly via USB.

![HTOTP](Preview/HTOTP.jpeg)

---

## Why I Built This

Got tired of pulling out my phone every time I need a 2FA code. This hardware authenticator lives on my desk and works instantly - press button, get OTP. Plus it's a fun way to learn about cryptography, embedded systems, and web protocols.

---

## Features

- **No phone required** - Standalone TOTP generator
- **USB keyboard emulation** - Types OTP directly into any field
- **Multiple accounts** - Store up to 10 TOTP secrets
- **Offline capable** - Works without WiFi using RTC backup
- **Web setup** - Configure via captive portal (no app needed)
- **Encrypted storage** - Secrets encrypted in flash memory

---

## Hardware

- ESP32-S3 (USB-native, WiFi)
- 128x64 OLED display (I2C)
- 2 buttons (next/paste)
- DS3231 RTC module (optional - for offline timekeeping)

<img src="Preview/layout.png" width="500" alt="Pin-Layout">

| Component | Pin | Description |
|-----------|-----|-------------|
| OLED SDA  | 8   | I²C Data |
| OLED SCL  | 9   | I²C Clock |
| Button NEXT | 4 | Cycle accounts |
| Button PASTE | 5 | Type OTP |
| RTC SDA | 8 | Shared I²C (optional) |
| RTC SCL | 9 | Shared I²C (optional) |

---

## Quick Start

1. Install **RTClib** via Arduino Library Manager
2. Upload to ESP32-S3
3. Hold **both buttons** at boot → enters setup mode
4. Connect to WiFi **"TOTP-Setup-XXXX"** (password: `totpsetup`)
5. Configure WiFi and add accounts via web browser
6. Device reboots - ready to use!

See [BUILD.md](BUILD.md) for detailed instructions.

---

## Usage

- **NEXT button** - Cycle through accounts
- **PASTE button** - Type current OTP via USB
- Display shows: account name, OTP code, time remaining

---

## Tech Stack

**Crypto:** HMAC-SHA1, Base32 decoding, AES-256 encryption
**Network:** WiFi, NTP, captive portal (DNS + HTTP)
**Storage:** ESP32 NVS (encrypted flash)
**Hardware:** I2C (display + RTC), USB HID

---

## How It Works

1. Syncs time via NTP (or uses RTC if offline)
2. Generates 6-digit TOTP using HMAC-SHA1(secret, timestamp)
3. Displays on OLED, types via USB keyboard when button pressed
4. Updates RTC from NTP every hour for offline fallback

---

## File Structure

```
totpgen.ino          - Main program (setup/loop)
config.h             - Pin definitions, constants
totp_core.cpp/.h     - TOTP/HOTP algorithms
storage.cpp/.h       - NVS + encryption
rtc_manager.cpp/.h   - Time source arbitration
web_provision.cpp/.h - Captive portal setup
buttons.cpp/.h       - Button debouncing
display_ui.cpp/.h    - OLED rendering
```

Arduino IDE automatically compiles all .cpp/.h files - no configuration needed!

---

## Security

- Secrets encrypted with AES-256 (device-unique key from MAC)
- Stored in NVS (flash memory)
- For production: enable ESP32 flash encryption (see BUILD.md)

---

## Demo

![Demo](Preview/demo_video.mp4)

---

## License

MIT - Do whatever you want with it.

---

## Notes

This was a learning project exploring embedded systems, cryptography, and wireless protocols. It works well for personal use but isn't production-hardened. If you build one, don't blame me if you get locked out of your accounts ;)

---

## Interview Prep

If you're using this project for technical interviews, see [INTERVIEW_PREP.md](INTERVIEW_PREP.md) for a comprehensive guide on explaining the architecture, algorithms, and design decisions.
