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
#include "config.h"
#include "totp_core.h"
#include "buttons.h"
#include "display_ui.h"
#include "storage.h"

// --- Storage Manager ---
StorageManager storage;
DeviceConfig deviceConfig;

// --- Wi-Fi Config (fallback if NVS empty) ---
const char* WIFI_SSID = "";
const char* WIFI_PASS = "";

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

  // Initialize storage
  if (storage.begin()) {
    Serial.println("NVS initialized");
    storage.loadConfig(&deviceConfig);
    if (deviceConfig.provisioned) {
      Serial.println("Using stored WiFi credentials");
    }
  } else {
    Serial.println("NVS init failed, using defaults");
  }

  // Wi-Fi + Time Sync
  const char* ssid = deviceConfig.wifi_ssid[0] ? deviceConfig.wifi_ssid : WIFI_SSID;
  const char* pass = deviceConfig.wifi_pass[0] ? deviceConfig.wifi_pass : WIFI_PASS;
  WiFi.begin(ssid, pass);
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
