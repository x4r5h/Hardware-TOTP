#include "rtc_manager.h"
#include <WiFi.h>

RTCManager::RTCManager() : rtc_present(false), last_ntp_sync(0) {}

bool RTCManager::begin() {
  if (!rtc.begin()) {
    rtc_present = false;
    return false;
  }

  rtc_present = true;

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  return true;
}

bool RTCManager::isRTCPresent() {
  return rtc_present;
}

time_t RTCManager::getTime() {
  if (WiFi.status() == WL_CONNECTED) {
    time_t ntp_time = time(nullptr);
    if (ntp_time > 1000000) {
      last_ntp_sync = millis();
      return ntp_time;
    }
  }

  if (rtc_present) {
    DateTime now = rtc.now();
    return now.unixtime();
  }

  return time(nullptr);
}

TimeSource RTCManager::getCurrentSource() {
  if (WiFi.status() == WL_CONNECTED) {
    time_t t = time(nullptr);
    if (t > 1000000) return TIME_NTP;
  }

  if (rtc_present) {
    return TIME_RTC;
  }

  return TIME_NONE;
}

bool RTCManager::syncRTCFromNTP() {
  if (!rtc_present) return false;

  time_t now = time(nullptr);
  if (now < 1000000) return false;

  rtc.adjust(DateTime(now));
  last_ntp_sync = millis();

  return true;
}

bool RTCManager::syncSystemFromRTC() {
  if (!rtc_present) return false;

  DateTime now = rtc.now();
  timeval tv = { now.unixtime(), 0 };
  settimeofday(&tv, nullptr);

  return true;
}

unsigned long RTCManager::getTimeSinceSync() {
  if (last_ntp_sync == 0) return 0xFFFFFFFF;
  return millis() - last_ntp_sync;
}
