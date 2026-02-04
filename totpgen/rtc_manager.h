#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <Arduino.h>
#include <RTClib.h>

enum TimeSource {
  TIME_NONE,
  TIME_NTP,
  TIME_RTC
};

class RTCManager {
private:
  RTC_DS3231 rtc;
  bool rtc_present;
  unsigned long last_ntp_sync;

public:
  RTCManager();

  bool begin();
  bool isRTCPresent();

  time_t getTime();
  TimeSource getCurrentSource();

  bool syncRTCFromNTP();
  bool syncSystemFromRTC();

  unsigned long getTimeSinceSync();
};

#endif
