/*
  ESP32 Hardware TOTP Generator
  Author: Arsh Sanghavi
  Description:
    Generates 6-digit time-based OTPs (TOTP) for multiple accounts,
    displays them on OLED, and sends them via USB keyboard.
    Uses Wi-Fi + NTP for accurate time sync.
*/

#include <WiFi.h>
#include <lwip/apps/sntp.h>
#include <mbedtls/md.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <Arduino.h>
#include "USB.h"
#include "USBHIDKeyboard.h"

// --- Wi-Fi and NTP Config ---
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 0;
const int DAYLIGHT_OFFSET_SEC = 0;
const int TOTP_INTERVAL = 30; // OTP changes every 30s

// --- OLED Pins ---
#define OLED_SDA 8
#define OLED_SCL 9

// --- Buttons ---
const int BTN_NEXT = 4;
const int BTN_PASTE = 5;
const unsigned long DEBOUNCE_DELAY = 50;

// --- UI Offsets ---
const int X_OFFSET = 2;

// --- OLED Display Object ---
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

// --- USB Keyboard Object ---
USBHIDKeyboard Keyboard;

// --- Account List ---
struct Account {
  const char* label;
  const char* secret; // base32 encoded secret
};

Account accountList[] = {
  {"Google", "JBSWY3DPEHPK3PXP"},
  {"GitHub", "NB2W45DFOIZA===="},
  {"Work",   "MZXW6YTBOI======"}
};
const int TOTAL_ACCOUNTS = sizeof(accountList) / sizeof(accountList[0]);

// --- Button States ---
int currentAccount = 0;
int lastNextState = HIGH;
unsigned long lastDebounce = 0;
int lastPasteState = HIGH;
bool pasteTriggered = false;

// ======================================================
//              HELPER FUNCTIONS
// ======================================================

// Convert Base32 character to value
int base32CharValue(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= '2' && c <= '7') return c - '2' + 26;
  return -1;
}

// Decode Base32 string into bytes
int decodeBase32(const char *encoded, uint8_t *output, int maxLen) {
  int buffer = 0, bitsLeft = 0, count = 0;
  for (; *encoded && count < maxLen; ++encoded) {
    int val = base32CharValue(toupper(*encoded));
    if (val < 0) continue;
    buffer = (buffer << 5) | (val & 0x1F);
    bitsLeft += 5;
    if (bitsLeft >= 8) {
      output[count++] = (buffer >> (bitsLeft - 8)) & 0xFF;
      bitsLeft -= 8;
    }
  }
  return count;
}

// Compute HMAC-SHA1
void computeHMAC(const uint8_t *key, int keyLen, const uint8_t *msg, int msgLen, uint8_t *result) {
  mbedtls_md_context_t ctx;
  const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, info, 1);
  mbedtls_md_hmac_starts(&ctx, key, keyLen);
  mbedtls_md_hmac_update(&ctx, msg, msgLen);
  mbedtls_md_hmac_finish(&ctx, result);
  mbedtls_md_free(&ctx);
}

// HOTP Generator
uint32_t generateHOTP(const char *base32Secret, uint64_t counter) {
  uint8_t key[64];
  int keyLen = decodeBase32(base32Secret, key, sizeof(key));

  uint8_t msg[8];
  for (int i = 7; i >= 0; i--) {
    msg[i] = counter & 0xFF;
    counter >>= 8;
  }

  uint8_t hash[20];
  computeHMAC(key, keyLen, msg, 8, hash);

  int offset = hash[19] & 0x0F;
  uint32_t code = ((hash[offset] & 0x7f) << 24) |
                  ((hash[offset + 1] & 0xff) << 16) |
                  ((hash[offset + 2] & 0xff) << 8) |
                  (hash[offset + 3] & 0xff);

  return code % 1000000;
}

// TOTP Generator
uint32_t generateTOTP(const char *secret) {
  time_t now = time(nullptr);
  uint64_t counter = now / TOTP_INTERVAL;
  return generateHOTP(secret, counter);
}

// Progress bar for remaining time
void drawProgress(int x, int y, int w, int h, int percent) {
  display.drawFrame(x, y, w, h);
  int fillWidth = map(percent, 0, 100, 0, w - 2);
  if (fillWidth > 0) display.drawBox(x + 1, y + 1, fillWidth, h - 2);
}

// ======================================================
//              BUTTON HANDLERS
// ======================================================

void initButtons() {
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PASTE, INPUT_PULLUP);
}

bool isNextPressed() {
  int reading = digitalRead(BTN_NEXT);
  if (reading != lastNextState) lastDebounce = millis();
  if ((millis() - lastDebounce) > DEBOUNCE_DELAY) {
    static int stable = HIGH;
    if (reading != stable) {
      stable = reading;
      if (stable == LOW) {
        lastNextState = reading;
        return true;
      }
    }
  }
  lastNextState = reading;
  return false;
}

bool isPastePressed() {
  int reading = digitalRead(BTN_PASTE);
  if (lastPasteState == HIGH && reading == LOW && !pasteTriggered) {
    pasteTriggered = true;
    lastPasteState = reading;
    return true;
  }
  if (lastPasteState == LOW && reading == HIGH) pasteTriggered = false;
  lastPasteState = reading;
  return false;
}

// ======================================================
//              MAIN SETUP
// ======================================================

void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin();
  initButtons();
  USB.begin();
  Keyboard.begin();

  // Wi-Fi + Time Sync
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to Wi-Fi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(300);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  } else {
    Serial.println("\nWi-Fi not connected, using default time.");
  }

  // Intro screen
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(X_OFFSET, 14, "Hardware TOTP Generator");
  display.drawStr(X_OFFSET, 32, "by Arsh Sanghavi");
  display.sendBuffer();
  delay(3000);
}

// ======================================================
//              MAIN LOOP
// ======================================================

unsigned long lastUpdate = 0;

void loop() {
  if (isNextPressed()) {
    currentAccount = (currentAccount + 1) % TOTAL_ACCOUNTS;
    Serial.printf("Switched to: %s\n", accountList[currentAccount].label);
    lastUpdate = 0;
  }

  if (isPastePressed()) {
    uint32_t otp = generateTOTP(accountList[currentAccount].secret);
    char otpStr[8];
    sprintf(otpStr, "%06u", otp);
    Keyboard.print(otpStr);
    Serial.printf("Pasted OTP for %s: %s\n", accountList[currentAccount].label, otpStr);
  }

  if (millis() - lastUpdate < 250) return;
  lastUpdate = millis();

  time_t now = time(nullptr);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char timeStr[16];
  if (now < 1000000) strcpy(timeStr, "No Time");
  else strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

  uint32_t code = 0;
  int percent = 0;
  int secondsLeft = 0;

  if (now >= 1000000) {
    code = generateTOTP(accountList[currentAccount].secret);
    int sec = now % TOTP_INTERVAL;
    secondsLeft = TOTP_INTERVAL - sec;
    percent = (secondsLeft * 100) / TOTP_INTERVAL;
  }

  // --- Update OLED Display ---
  display.clearBuffer();
  display.setFont(u8g2_font_6x12_tr);
  display.drawStr(X_OFFSET, 10, accountList[currentAccount].label);
  display.setCursor(80, 10);
  display.print(timeStr);

  char codeStr[10];
  if (now < 1000000) {
    strcpy(codeStr, "------");
    display.drawStr(X_OFFSET, 32, "Waiting for NTP...");
  } else {
    sprintf(codeStr, "%06u", code);
    display.setFont(u8g2_font_logisoso22_tr);
    display.drawStr(8, 50, codeStr);
  }

  drawProgress(8, 56, 112, 6, percent);
  display.setFont(u8g2_font_6x12_tr);
  char secStr[10];
  sprintf(secStr, "%2ds", secondsLeft);
  display.drawStr(98, 54, secStr);
  display.sendBuffer();
}
