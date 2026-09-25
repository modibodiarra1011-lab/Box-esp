/*
 * Projet 14 - Capteur de luminosite BH1750 (lux, I2C)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "BH1750" (par claws / Christopher Laws)
 */

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter(0x23); // 0x23 (ADDR a GND) ou 0x5C (ADDR a VCC)
const unsigned long READ_INTERVAL_MS = 1000;
unsigned long lastReadTime = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("[14_bh1750] Capteur BH1750 initialise.");
  } else {
    Serial.println("[14_bh1750] Echec d'initialisation du BH1750 !");
  }
}

void loop() {
  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL_MS) {
    lastReadTime = now;
    float lux = lightMeter.readLightLevel();
    if (lux < 0) {
      Serial.println("[14_bh1750] Erreur de lecture.");
    } else {
      Serial.printf("[14_bh1750] Luminosite = %.1f lux\n", lux);
    }
  }
}
