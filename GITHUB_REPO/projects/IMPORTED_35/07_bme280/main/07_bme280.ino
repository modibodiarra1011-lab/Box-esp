/*
 * Projet 07 - Capteur BME280 (temperature / humidite / pression)
 * Cible : ESP32 / Uno (I2C)
 * Bibliotheque requise : "Adafruit BME280 Library" + "Adafruit Unified Sensor"
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

#define BME280_I2C_ADDRESS 0x76 // 0x77 si la broche SDO est tiree au VCC

Adafruit_BME280 bme;
const unsigned long READ_INTERVAL_MS = 1000;
unsigned long lastReadTime = 0;
bool sensorFound = false;

const float SEA_LEVEL_HPA = 1013.25; // ajuster selon votre pression locale de reference

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);

  sensorFound = bme.begin(BME280_I2C_ADDRESS, &Wire);
  if (!sensorFound) {
    Serial.println("[07_bme280] BME280 non trouve ! Verifiez le cablage/adresse I2C.");
  } else {
    Serial.println("[07_bme280] Capteur detecte et initialise.");
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                     Adafruit_BME280::SAMPLING_X2,  // temperature
                     Adafruit_BME280::SAMPLING_X16, // pression
                     Adafruit_BME280::SAMPLING_X1,  // humidite
                     Adafruit_BME280::FILTER_X16,
                     Adafruit_BME280::STANDBY_MS_500);
  }
}

void loop() {
  if (!sensorFound) return;
  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL_MS) {
    lastReadTime = now;
    float tempC = bme.readTemperature();
    float pressureHpa = bme.readPressure() / 100.0F;
    float humidity = bme.readHumidity();
    float altitude = bme.readAltitude(SEA_LEVEL_HPA);

    Serial.printf("[07_bme280] T=%.2fC  P=%.2fhPa  H=%.2f%%  Alt=%.1fm\n",
                  tempC, pressureHpa, humidity, altitude);
  }
}
