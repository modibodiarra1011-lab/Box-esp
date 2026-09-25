/*
 * Projet 05 - Scanner I2C
 * Cible : ESP32 / Uno
 * But : détecter automatiquement les adresses I2C occupées sur le bus,
 * essentiel pour diagnostiquer les conflits d'adresses (voir wiring.md
 * et les projets combinés 31+ qui utilisent plusieurs capteurs I2C).
 */

#include <Wire.h>

const uint8_t SDA_PIN = 21; // par defaut ESP32 (A4 sur Uno)
const uint8_t SCL_PIN = 22; // par defaut ESP32 (A5 sur Uno)
const unsigned long SCAN_INTERVAL_MS = 5000;
unsigned long lastScan = 0;

void scanBus() {
  Serial.println("[05_i2c_scanner] --- Debut du scan I2C ---");
  int found = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("  Peripherique trouve a l'adresse 0x%02X\n", address);
      found++;
    } else if (error == 4) {
      Serial.printf("  Erreur inconnue a l'adresse 0x%02X\n", address);
    }
  }
  if (found == 0) {
    Serial.println("  Aucun peripherique I2C detecte.");
  } else {
    Serial.printf("[05_i2c_scanner] %d peripherique(s) detecte(s).\n", found);
  }
  Serial.println("[05_i2c_scanner] --- Fin du scan ---");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000); // 100 kHz standard, fiable pour de longs cables
  Serial.println("[05_i2c_scanner] Pret");
  scanBus();
  lastScan = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastScan >= SCAN_INTERVAL_MS) {
    lastScan = now;
    scanBus();
  }
}
