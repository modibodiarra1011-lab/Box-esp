/*
 * Projet 06 - Capteur DHT22 (temperature / humidite)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "DHT sensor library" (Adafruit) + "Adafruit Unified Sensor"
 * But : lire temperature et humidite relative de facon non bloquante.
 */

#include <Arduino.h>
#include <DHT.h>

const uint8_t DHT_PIN = 4;
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);

const unsigned long READ_INTERVAL_MS = 2500; // le DHT22 ne supporte pas plus de 0.5Hz
unsigned long lastReadTime = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  dht.begin();
  Serial.println("[06_dht22] Pret");
}

void loop() {
  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL_MS) {
    lastReadTime = now;
    float humidity = dht.readHumidity();
    float tempC = dht.readTemperature();

    if (isnan(humidity) || isnan(tempC)) {
      Serial.println("[06_dht22] Erreur de lecture du capteur DHT22 !");
      return;
    }

    float heatIndex = dht.computeHeatIndex(tempC, humidity, false);
    Serial.printf("[06_dht22] T=%.1fC  H=%.1f%%  IndiceChaleur=%.1fC\n",
                  tempC, humidity, heatIndex);
  }
}
