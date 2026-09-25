/*
 * Projet 12 - Servomoteur SG90
 * Cible : ESP32 (bibliotheque ESP32Servo car la lib Servo standard AVR
 * n'est pas compatible ESP32) / Uno (bibliotheque Servo standard)
 * Bibliotheque requise : "ESP32Servo" (par Kevin Harrington / madhephaestus)
 */

#include <Arduino.h>
#include <ESP32Servo.h>

Servo myServo;
const uint8_t SERVO_PIN = 18;

const unsigned long SWEEP_INTERVAL_MS = 20; // vitesse de balayage
unsigned long lastStepTime = 0;
int currentAngle = 0;
int stepDirection = 1;

void setup() {
  Serial.begin(115200);
  delay(200);

  // Configuration PWM specifique ESP32 (frequence/timers geres par la lib)
  ESP32PWM::allocateTimer(0);
  myServo.setPeriodHertz(50);          // frequence standard servo analogique 50Hz
  myServo.attach(SERVO_PIN, 500, 2400); // pulses min/max en microsecondes (calibrage SG90 typique)

  Serial.println("[12_sg90] Pret - balayage 0 a 180 degres");
}

void loop() {
  unsigned long now = millis();
  if (now - lastStepTime >= SWEEP_INTERVAL_MS) {
    lastStepTime = now;
    currentAngle += stepDirection;

    if (currentAngle >= 180) {
      currentAngle = 180;
      stepDirection = -1;
    } else if (currentAngle <= 0) {
      currentAngle = 0;
      stepDirection = 1;
    }

    myServo.write(currentAngle);
  }
}
