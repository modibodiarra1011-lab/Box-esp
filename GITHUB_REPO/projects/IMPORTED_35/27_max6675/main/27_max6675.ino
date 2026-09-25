/*
 * Projet 27 - Thermocouple type K via MAX6675 (SPI, lecture seule)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "MAX6675" (par Adafruit ou compatible)
 */

#include <Arduino.h>
#include <SPI.h>
#include <max6675.h>

const uint8_t MAX6675_SCK_PIN = 18;
const uint8_t MAX6675_CS_PIN  = 15;
const uint8_t MAX6675_SO_PIN  = 19;

MAX6675 thermocouple(MAX6675_SCK_PIN, MAX6675_CS_PIN, MAX6675_SO_PIN);

const unsigned long READ_INTERVAL_MS = 500; // le MAX6675 se rafraichit toutes les ~220ms en interne
unsigned long lastReadTime = 0;

void setup() {
  Serial.begin(115200);
  delay(500); // le MAX6675 necessite un court delai de stabilisation au demarrage
  Serial.println("[27_max6675] Pret");
}

void loop() {
  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL_MS) {
    lastReadTime = now;
    double tempC = thermocouple.readCelsius();

    if (isnan(tempC)) {
      Serial.println("[27_max6675] Erreur de lecture (thermocouple deconnecte ?)");
    } else {
      Serial.printf("[27_max6675] Temperature thermocouple = %.2f C\n", tempC);
    }
  }
}
