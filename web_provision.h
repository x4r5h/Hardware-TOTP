#ifndef WEB_PROVISION_H
#define WEB_PROVISION_H

#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "config.h"
#include "storage.h"

class WebProvisioning {
private:
  WebServer* server;
  DNSServer* dns;
  StorageManager* storage;
  bool ap_active;
  unsigned long start_time;

  void handleRoot();
  void handleWiFi();
  void handleWiFiSave();
  void handleAddAccount();
  void handleSaveAccount();
  void handleAccounts();
  void handleDelete();
  void handleReboot();
  void handleNotFound();

  String getHeader();
  String getFooter();
  bool isValidBase32(const char* str);

public:
  WebProvisioning(StorageManager* store);

  bool begin();
  void stop();
  void handle();
  bool isActive();
};

#endif
