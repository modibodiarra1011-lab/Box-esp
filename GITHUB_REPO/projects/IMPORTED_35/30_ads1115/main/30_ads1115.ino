/*
 * Projet 30 - ADC externe 16 bits ADS1115 (I2C, 4 canaux)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "Adafruit ADS1X15"
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;
const unsigned long READ_INTERVAL_MS = 500;
unsigned long lastReadTime = 0;

// Facteur de conversion : depend du gain choisi (GAIN_TWOTHIRDS => +/-6.144V, 0.1875mV/bit)
const float VOLTS_PER_BIT = 0.1875F / 1000.0F;

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);

  if (!ads.begin(0x48, &Wire)) {
    Serial.println("[30_ads1115] ADS1115 non trouve ! Verifiez cablage/adresse.");
    while (true) { delay(1000); }
  }

  ads.setGain(GAIN_TWOTHIRDS); // +/- 6.144V range (adapte a des signaux jusqu'a ~6V max)
  Serial.println("[30_ads1115] ADC 16 bits initialise - lecture des 4 canaux");
}

void loop() {
  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL_MS) {
    lastReadTime = now;

    for (uint8_t channel = 0; channel < 4; channel++) {
      int16_t raw = ads.readADC_SingleEnded(channel);
      float voltage = raw * VOLTS_PER_BIT;
      Serial.printf("[30_ads1115] Canal A%d : raw=%6d  V=%.4fV\n", channel, raw, voltage);
    }
    Serial.println("---");
  }
}
