/*
 * Projet 04 - Scan des réseaux WiFi
 * Cible : ESP32 (WiFi natif). Non applicable sur Arduino Uno sans module WiFi externe.
 * But : lister les réseaux WiFi visibles avec SSID, RSSI et type de sécurité.
 */

#include <WiFi.h>

const unsigned long SCAN_INTERVAL_MS = 10000;
unsigned long lastScanTime = 0;

String encryptionTypeToString(wifi_auth_mode_t authMode) {
  switch (authMode) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK: return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2_ENTERPRISE";
    case WIFI_AUTH_WPA3_PSK: return "WPA3_PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2_WPA3_PSK";
    default: return "UNKNOWN";
  }
}

void doScan() {
  Serial.println("[04_wifi_scan] Scan en cours...");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("[04_wifi_scan] Aucun reseau trouve.");
    return;
  }
  Serial.printf("[04_wifi_scan] %d reseau(x) trouve(s):\n", n);
  for (int i = 0; i < n; i++) {
    Serial.printf("  %2d) SSID=%-32s RSSI=%4ddBm Canal=%2d Secu=%s\n",
                  i + 1,
                  WiFi.SSID(i).c_str(),
                  WiFi.RSSI(i),
                  WiFi.channel(i),
                  encryptionTypeToString(WiFi.encryptionType(i)).c_str());
  }
  WiFi.scanDelete(); // libere la memoire du scan
}

void setup() {
  Serial.begin(115200);
  delay(200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // s'assure qu'on n'est pas deja connecte pendant le scan
  delay(100);
  Serial.println("[04_wifi_scan] Pret");
  doScan();
  lastScanTime = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastScanTime >= SCAN_INTERVAL_MS) {
    lastScanTime = now;
    doScan();
  }
}
