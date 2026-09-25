/*
 * Projet 11 - Capteur DS18B20 (temperature, bus OneWire)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "OneWire" + "DallasTemperature"
 */

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const uint8_t ONE_WIRE_PIN = 4;
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

const unsigned long READ_INTERVAL_MS = 1000; // en mode non bloquant, >= temps de conversion
unsigned long lastRequestTime = 0;
bool conversionInProgress = false;
int deviceCount = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  sensors.begin();
  sensors.setWaitForConversion(false); // mode non bloquant : on gere le delai nous-memes

  deviceCount = sensors.getDeviceCount();
  Serial.printf("[11_ds18b20] %d capteur(s) DS18B20 detecte(s) sur le bus.\n", deviceCount);
  if (deviceCount == 0) {
    Serial.println("[11_ds18b20] Verifiez le cablage et la resistance de pull-up.");
  }
}

void loop() {
  unsigned long now = millis();

  if (!conversionInProgress && (now - lastRequestTime >= READ_INTERVAL_MS)) {
    sensors.requestTemperatures(); // lance la conversion (asynchrone)
    conversionInProgress = true;
    lastRequestTime = now;
  }

  // Temps de conversion max pour resolution 12 bits : 750 ms
  if (conversionInProgress && (now - lastRequestTime >= 750)) {
    conversionInProgress = false;
    for (int i = 0; i < deviceCount; i++) {
      float tempC = sensors.getTempCByIndex(i);
      if (tempC == DEVICE_DISCONNECTED_C) {
        Serial.printf("[11_ds18b20] Capteur %d deconnecte !\n", i);
      } else {
        Serial.printf("[11_ds18b20] Capteur %d : T=%.2fC\n", i, tempC);
      }
    }
  }
}
