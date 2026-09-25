/*
 * Projet 24 - Diagnostic systeme (Worker Diagnostic)
 * Cible : ESP32
 * But : afficher periodiquement un rapport de sante materiel/logiciel :
 * infos puce, memoire heap libre, tension/frequence CPU, MAC, temperature
 * interne (si disponible), et test du watchdog logiciel.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "esp_task_wdt.h"

const unsigned long REPORT_INTERVAL_MS = 5000;
unsigned long lastReportTime = 0;
uint32_t bootCount = 0;

// NOTE COMPATIBILITE : l'API du Task Watchdog a change de signature avec le
// core Arduino-ESP32 v3.x (base sur ESP-IDF 5.x). Ce code cible le core v3.x
// (esp_task_wdt_config_t + esp_task_wdt_init(&config)). Si vous utilisez un
// core v2.x, remplacez l'appel dans setup() par :
//   esp_task_wdt_init(10, true); esp_task_wdt_add(NULL);
// Voir ESP_ARDUINO_VERSION_MAJOR pour detecter automatiquement la version si besoin.

void printChipInfo() {
  Serial.println("========== [24_worker_diagnostic] Rapport systeme ==========");
  Serial.printf("Modele puce       : %s\n", ESP.getChipModel());
  Serial.printf("Coeurs CPU        : %d\n", ESP.getChipCores());
  Serial.printf("Revision silicium : %d\n", ESP.getChipRevision());
  Serial.printf("Frequence CPU     : %d MHz\n", getCpuFrequencyMhz());
  Serial.printf("Flash totale      : %u bytes\n", ESP.getFlashChipSize());
  Serial.printf("Heap libre        : %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Heap min historique: %u bytes\n", ESP.getMinFreeHeap());
  Serial.printf("Adresse MAC       : %s\n", WiFi.macAddress().c_str());
  Serial.printf("Uptime            : %lu s\n", millis() / 1000);
  Serial.printf("SDK version       : %s\n", ESP.getSdkVersion());

  // temperatureRead() est une fonction native du core Arduino-ESP32, disponible
  // sur ESP32, ESP32-S2, ESP32-S3, ESP32-C3 (capteur de temperature interne du SoC,
  // PAS une mesure ambiante fiable - voir wiring.md).
  float internalTempC = temperatureRead();
  Serial.printf("Temp. interne SoC : %.1f C\n", internalTempC);

  Serial.println("=============================================================");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  bootCount++;

  // Watchdog logiciel : redemarre la carte si loop() se bloque plus de 10s.
  // esp_task_wdt_deinit() est appele en premier car le core Arduino-ESP32 v3.x
  // initialise deja un watchdog par defaut sur certaines configurations ;
  // le desactiver d'abord evite une erreur "already initialized".
  esp_task_wdt_deinit();
  esp_task_wdt_config_t wdtConfig = {
    .timeout_ms = 10000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdtConfig);
  esp_task_wdt_add(NULL);

  WiFi.mode(WIFI_STA); // necessaire pour obtenir une adresse MAC valide sans se connecter

  Serial.println("[24_worker_diagnostic] Demarrage - watchdog actif (timeout 10s)");
  printChipInfo();
  lastReportTime = millis();
}

void loop() {
  esp_task_wdt_reset(); // "nourrit" le watchdog pour prouver que loop() tourne normalement

  unsigned long now = millis();
  if (now - lastReportTime >= REPORT_INTERVAL_MS) {
    lastReportTime = now;
    printChipInfo();
  }
}
