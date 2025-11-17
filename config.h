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

// --- Storage Configuration ---
#define MAX_ACCOUNTS 10
#define MAX_LABEL_LEN 24
#define MAX_SECRET_LEN 64
#define NVS_NAMESPACE "totp"

// Account structure
struct TOTPAccount {
  char label[MAX_LABEL_LEN];
  char secret[MAX_SECRET_LEN];
  uint8_t digits;
  uint32_t period;
  bool active;
};

// Device configuration
struct DeviceConfig {
  char wifi_ssid[33];
  char wifi_pass[65];
  int32_t gmt_offset;
  uint8_t account_count;
  bool provisioned;
};

#endif  // CONFIG_H
