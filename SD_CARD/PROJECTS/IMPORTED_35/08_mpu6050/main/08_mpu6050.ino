/*
 * Projet 08 - MPU6050 (accelerometre + gyroscope 6 axes)
 * Cible : ESP32 / Uno (I2C)
 * Bibliotheque requise : "Adafruit MPU6050" + "Adafruit Unified Sensor"
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;
const unsigned long READ_INTERVAL_MS = 200;
unsigned long lastReadTime = 0;
bool sensorFound = false;

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);

  sensorFound = mpu.begin(0x68, &Wire); // AD0 a GND -> 0x68 ; AD0 a VCC -> 0x69
  if (!sensorFound) {
    Serial.println("[08_mpu6050] MPU6050 non trouve ! Verifiez cablage/adresse.");
    return;
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("[08_mpu6050] Capteur detecte et configure.");
}

void loop() {
  if (!sensorFound) return;
  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL_MS) {
    lastReadTime = now;
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    Serial.printf("[08_mpu6050] Accel(m/s^2) X=%.2f Y=%.2f Z=%.2f | Gyro(rad/s) X=%.2f Y=%.2f Z=%.2f | T=%.1fC\n",
                  accel.acceleration.x, accel.acceleration.y, accel.acceleration.z,
                  gyro.gyro.x, gyro.gyro.y, gyro.gyro.z,
                  temp.temperature);
  }
}
