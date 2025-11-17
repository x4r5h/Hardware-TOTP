#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>
#include "config.h"

class StorageManager {
private:
  bool nvs_initialized;
  uint8_t device_key[32];

  void deriveDeviceKey();
  bool encryptSecret(const char* plain, char* encrypted, size_t encLen);
  bool decryptSecret(const char* encrypted, char* plain, size_t plainLen);

public:
  StorageManager();

  bool begin();
  bool isProvisioned();

  bool loadConfig(DeviceConfig* config);
  bool saveConfig(const DeviceConfig* config);

  bool loadAccount(uint8_t index, TOTPAccount* account);
  bool saveAccount(uint8_t index, const TOTPAccount* account);
  bool deleteAccount(uint8_t index);

  uint8_t getAccountCount();
  bool clearAll();
};

#endif
