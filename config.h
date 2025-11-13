#ifndef CONFIG_H
#define CONFIG_H

// ====================================
//     HARDWARE TOTP - CONFIGURATION
// ====================================

// --- NTP and Time Settings ---
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 0
#define DAYLIGHT_OFFSET_SEC 0
#define TOTP_INTERVAL 30  // OTP refresh interval in seconds

// --- OLED Display Pins ---
#define OLED_SDA 8
#define OLED_SCL 9

// --- Button Pins ---
#define BTN_NEXT 4
#define BTN_PASTE 5
#define DEBOUNCE_DELAY 50  // milliseconds

// --- UI Layout ---
#define X_OFFSET 2  // Left margin for text on display

#endif  // CONFIG_H
