#include "web_provision.h"
#include <WiFi.h>

WebProvisioning::WebProvisioning(StorageManager* store)
  : server(nullptr), dns(nullptr), storage(store), ap_active(false), start_time(0) {}

String WebProvisioning::getHeader() {
  return "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
         "<style>body{font-family:Arial;max-width:600px;margin:40px auto;padding:20px}"
         "input,button{width:100%;padding:10px;margin:8px 0;box-sizing:border-box}"
         "button{background:#4CAF50;color:white;border:none;cursor:pointer}"
         "button:hover{background:#45a049}.del{background:#f44336}.del:hover{background:#da190b}"
         "h1{color:#333}</style></head><body>";
}

String WebProvisioning::getFooter() {
  return "<hr><p><small>Hardware TOTP Generator</small></p></body></html>";
}

bool WebProvisioning::isValidBase32(const char* str) {
  for (int i = 0; str[i]; i++) {
    char c = toupper(str[i]);
    if (!((c >= 'A' && c <= 'Z') || (c >= '2' && c <= '7') || c == '=')) {
      return false;
    }
  }
  return true;
}

void WebProvisioning::handleRoot() {
  String html = getHeader();
  html += "<h1>TOTP Setup</h1>";
  html += "<p><a href='/wifi'><button>Configure WiFi</button></a></p>";
  html += "<p><a href='/add'><button>Add TOTP Account</button></a></p>";
  html += "<p><a href='/accounts'><button>Manage Accounts</button></a></p>";
  html += getFooter();
  server->send(200, "text/html", html);
}

void WebProvisioning::handleWiFi() {
  String html = getHeader();
  html += "<h1>WiFi Configuration</h1>";
  html += "<form method='POST' action='/wifi/save'>";
  html += "SSID: <input name='ssid' maxlength='32'><br>";
  html += "Password: <input type='password' name='pass' maxlength='64'><br>";
  html += "<button type='submit'>Save & Reboot</button>";
  html += "</form><p><a href='/'>Back</a></p>";
  html += getFooter();
  server->send(200, "text/html", html);
}

void WebProvisioning::handleWiFiSave() {
  DeviceConfig config;
  storage->loadConfig(&config);

  server->arg("ssid").toCharArray(config.wifi_ssid, sizeof(config.wifi_ssid));
  server->arg("pass").toCharArray(config.wifi_pass, sizeof(config.wifi_pass));
  config.provisioned = true;

  storage->saveConfig(&config);

  String html = getHeader();
  html += "<h1>WiFi Saved!</h1>";
  html += "<p>Device will reboot in 3 seconds...</p>";
  html += getFooter();
  server->send(200, "text/html", html);

  delay(3000);
  ESP.restart();
}

void WebProvisioning::handleAddAccount() {
  String html = getHeader();
  html += "<h1>Add TOTP Account</h1>";
  html += "<form method='POST' action='/account/save'>";
  html += "Label: <input name='label' maxlength='23'><br>";
  html += "Secret (Base32): <input name='secret' maxlength='63'><br>";
  html += "<button type='submit'>Add Account</button>";
  html += "</form><p><a href='/'>Back</a></p>";
  html += getFooter();
  server->send(200, "text/html", html);
}

void WebProvisioning::handleSaveAccount() {
  String label = server->arg("label");
  String secret = server->arg("secret");

  if (!isValidBase32(secret.c_str())) {
    String html = getHeader();
    html += "<h1>Error</h1><p>Invalid Base32 secret!</p>";
    html += "<p><a href='/add'>Try Again</a></p>" + getFooter();
    server->send(400, "text/html", html);
    return;
  }

  DeviceConfig config;
  storage->loadConfig(&config);

  if (config.account_count >= MAX_ACCOUNTS) {
    String html = getHeader();
    html += "<h1>Error</h1><p>Maximum accounts reached!</p>";
    html += "<p><a href='/'>Back</a></p>" + getFooter();
    server->send(400, "text/html", html);
    return;
  }

  TOTPAccount account = {0};
  label.toCharArray(account.label, sizeof(account.label));
  secret.toCharArray(account.secret, sizeof(account.secret));
  account.digits = 6;
  account.period = 30;
  account.active = true;

  storage->saveAccount(config.account_count, &account);
  config.account_count++;
  storage->saveConfig(&config);

  String html = getHeader();
  html += "<h1>Account Added!</h1>";
  html += "<p><a href='/accounts'><button>View Accounts</button></a></p>";
  html += "<p><a href='/add'><button>Add Another</button></a></p>";
  html += getFooter();
  server->send(200, "text/html", html);
}

void WebProvisioning::handleAccounts() {
  DeviceConfig config;
  storage->loadConfig(&config);

  String html = getHeader();
  html += "<h1>Accounts (" + String(config.account_count) + ")</h1>";

  for (uint8_t i = 0; i < config.account_count; i++) {
    TOTPAccount acc;
    if (storage->loadAccount(i, &acc)) {
      html += "<div style='border:1px solid #ddd;padding:10px;margin:10px 0'>";
      html += "<b>" + String(acc.label) + "</b><br>";
      html += "<form method='POST' action='/delete' style='display:inline'>";
      html += "<input type='hidden' name='index' value='" + String(i) + "'>";
      html += "<button type='submit' class='del'>Delete</button>";
      html += "</form></div>";
    }
  }

  html += "<p><a href='/'><button>Back</button></a></p>";
  html += getFooter();
  server->send(200, "text/html", html);
}

void WebProvisioning::handleDelete() {
  int index = server->arg("index").toInt();

  DeviceConfig config;
  storage->loadConfig(&config);

  if (index >= 0 && index < config.account_count) {
    for (int i = index; i < config.account_count - 1; i++) {
      TOTPAccount acc;
      storage->loadAccount(i + 1, &acc);
      storage->saveAccount(i, &acc);
    }

    storage->deleteAccount(config.account_count - 1);
    config.account_count--;
    storage->saveConfig(&config);
  }

  server->sendHeader("Location", "/accounts");
  server->send(303);
}

void WebProvisioning::handleReboot() {
  String html = getHeader();
  html += "<h1>Rebooting...</h1>";
  html += getFooter();
  server->send(200, "text/html", html);
  delay(1000);
  ESP.restart();
}

void WebProvisioning::handleNotFound() {
  server->sendHeader("Location", "/");
  server->send(302);
}

bool WebProvisioning::begin() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char ap_name[32];
  sprintf(ap_name, "TOTP-Setup-%02X%02X", mac[4], mac[5]);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_name, "totpsetup");

  delay(500);

  dns = new DNSServer();
  dns->start(53, "*", WiFi.softAPIP());

  server = new WebServer(80);

  server->on("/", [this]() { handleRoot(); });
  server->on("/wifi", [this]() { handleWiFi(); });
  server->on("/wifi/save", HTTP_POST, [this]() { handleWiFiSave(); });
  server->on("/add", [this]() { handleAddAccount(); });
  server->on("/account/save", HTTP_POST, [this]() { handleSaveAccount(); });
  server->on("/accounts", [this]() { handleAccounts(); });
  server->on("/delete", HTTP_POST, [this]() { handleDelete(); });
  server->on("/reboot", [this]() { handleReboot(); });
  server->onNotFound([this]() { handleNotFound(); });

  server->begin();

  ap_active = true;
  start_time = millis();

  Serial.println("AP Mode: " + String(ap_name));
  Serial.println("IP: " + WiFi.softAPIP().toString());

  return true;
}

void WebProvisioning::stop() {
  if (server) {
    server->stop();
    delete server;
    server = nullptr;
  }

  if (dns) {
    dns->stop();
    delete dns;
    dns = nullptr;
  }

  WiFi.mode(WIFI_STA);
  ap_active = false;
}

void WebProvisioning::handle() {
  if (ap_active) {
    dns->processNextRequest();
    server->handleClient();
  }
}

bool WebProvisioning::isActive() {
  return ap_active;
}
