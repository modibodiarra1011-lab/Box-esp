/*
 * Projet 15 - Capteur de distance laser ToF VL53L0X (I2C)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "Adafruit VL53L0X"
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
const unsigned long READ_INTERVAL_MS = 200;
unsigned long lastReadTime = 0;
bool sensorFound = false;

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);

  // lox.begin() utilise l'adresse par defaut 0x29 et le bus Wire deja initialise ci-dessus.
  // Pour une adresse personnalisee (montage multi-capteurs), utilisez lox.begin(nouvelle_adresse).
  sensorFound = lox.begin();
  if (!sensorFound) {
    Serial.println("[15_vl53l0x] VL53L0X non trouve ! Verifiez cablage/adresse.");
  } else {
    Serial.println("[15_vl53l0x] Capteur ToF initialise.");
  }
}

void loop() {
  if (!sensorFound) return;
  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL_MS) {
    lastReadTime = now;

    VL53L0X_RangingMeasurementData_t measure;
    lox.rangingTest(&measure, false);

    if (measure.RangeStatus != 4) { // 4 = hors de portee / mesure invalide
      Serial.printf("[15_vl53l0x] Distance = %d mm\n", measure.RangeMilliMeter);
    } else {
      Serial.println("[15_vl53l0x] Hors de portee");
    }
  }
}
