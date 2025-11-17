#include "storage.h"
#include <Preferences.h>
#include <mbedtls/aes.h>
#include <esp_system.h>

Preferences prefs;

StorageManager::StorageManager() : nvs_initialized(false) {
  memset(device_key, 0, sizeof(device_key));
}

void StorageManager::deriveDeviceKey() {
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);

  for (int i = 0; i < 32; i++) {
    device_key[i] = mac[i % 6] ^ (i * 37);
  }
}

bool StorageManager::begin() {
  if (!prefs.begin(NVS_NAMESPACE, false)) {
    return false;
  }
  nvs_initialized = true;
  deriveDeviceKey();
  return true;
}

bool StorageManager::isProvisioned() {
  if (!nvs_initialized) return false;
  return prefs.getBool("prov", false);
}

bool StorageManager::loadConfig(DeviceConfig* config) {
  if (!nvs_initialized) return false;

  prefs.getString("ssid", config->wifi_ssid, sizeof(config->wifi_ssid));
  prefs.getString("pass", config->wifi_pass, sizeof(config->wifi_pass));
  config->gmt_offset = prefs.getInt("gmt", 0);
  config->account_count = prefs.getUChar("acnt", 0);
  config->provisioned = prefs.getBool("prov", false);

  return true;
}

bool StorageManager::saveConfig(const DeviceConfig* config) {
  if (!nvs_initialized) return false;

  prefs.putString("ssid", config->wifi_ssid);
  prefs.putString("pass", config->wifi_pass);
  prefs.putInt("gmt", config->gmt_offset);
  prefs.putUChar("acnt", config->account_count);
  prefs.putBool("prov", config->provisioned);

  return true;
}

bool StorageManager::encryptSecret(const char* plain, char* encrypted, size_t encLen) {
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_enc(&aes, device_key, 256);

  uint8_t input[16] = {0};
  uint8_t output[16] = {0};
  strncpy((char*)input, plain, 15);

  int ret = mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, output);
  mbedtls_aes_free(&aes);

  if (ret != 0) return false;

  for (int i = 0; i < 16 && i*2+1 < encLen; i++) {
    sprintf(&encrypted[i*2], "%02x", output[i]);
  }

  return true;
}

bool StorageManager::decryptSecret(const char* encrypted, char* plain, size_t plainLen) {
  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);
  mbedtls_aes_setkey_dec(&aes, device_key, 256);

  uint8_t input[16] = {0};
  uint8_t output[16] = {0};

  for (int i = 0; i < 16; i++) {
    sscanf(&encrypted[i*2], "%2hhx", &input[i]);
  }

  int ret = mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, input, output);
  mbedtls_aes_free(&aes);

  if (ret != 0) return false;

  strncpy(plain, (char*)output, plainLen - 1);
  plain[plainLen - 1] = '\0';

  return true;
}

bool StorageManager::loadAccount(uint8_t index, TOTPAccount* account) {
  if (!nvs_initialized || index >= MAX_ACCOUNTS) return false;

  char key[8];
  sprintf(key, "a%d_l", index);
  if (!prefs.isKey(key)) return false;

  prefs.getString(key, account->label, sizeof(account->label));

  sprintf(key, "a%d_s", index);
  char encSecret[65];
  prefs.getString(key, encSecret, sizeof(encSecret));
  decryptSecret(encSecret, account->secret, sizeof(account->secret));

  sprintf(key, "a%d_d", index);
  account->digits = prefs.getUChar(key, 6);

  sprintf(key, "a%d_p", index);
  account->period = prefs.getUInt(key, 30);

  account->active = true;
  return true;
}

bool StorageManager::saveAccount(uint8_t index, const TOTPAccount* account) {
  if (!nvs_initialized || index >= MAX_ACCOUNTS) return false;

  char key[8];
  sprintf(key, "a%d_l", index);
  prefs.putString(key, account->label);

  char encSecret[65] = {0};
  encryptSecret(account->secret, encSecret, sizeof(encSecret));
  sprintf(key, "a%d_s", index);
  prefs.putString(key, encSecret);

  sprintf(key, "a%d_d", index);
  prefs.putUChar(key, account->digits);

  sprintf(key, "a%d_p", index);
  prefs.putUInt(key, account->period);

  return true;
}

bool StorageManager::deleteAccount(uint8_t index) {
  if (!nvs_initialized || index >= MAX_ACCOUNTS) return false;

  char key[8];
  sprintf(key, "a%d_l", index);
  prefs.remove(key);
  sprintf(key, "a%d_s", index);
  prefs.remove(key);
  sprintf(key, "a%d_d", index);
  prefs.remove(key);
  sprintf(key, "a%d_p", index);
  prefs.remove(key);

  return true;
}

uint8_t StorageManager::getAccountCount() {
  if (!nvs_initialized) return 0;
  return prefs.getUChar("acnt", 0);
}

bool StorageManager::clearAll() {
  if (!nvs_initialized) return false;
  return prefs.clear();
}
