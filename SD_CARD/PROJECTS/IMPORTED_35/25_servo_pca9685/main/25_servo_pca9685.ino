/*
 * Projet 25 - Pilotage multi-servos via PCA9685 (I2C, 16 canaux PWM)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "Adafruit PWM Servo Driver Library"
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

#define SERVO_FREQ 50 // frequence standard pour servos analogiques
#define SERVO_MIN_PULSE 102  // ~500us a 50Hz (12 bits -> 4096 pas / 20ms)
#define SERVO_MAX_PULSE 512  // ~2500us a 50Hz

const uint8_t NUM_SERVOS = 4;
const unsigned long SWEEP_INTERVAL_MS = 15;
unsigned long lastStepTime = 0;
int angles[NUM_SERVOS] = {0, 45, 90, 135};
int directions[NUM_SERVOS] = {1, 1, -1, -1};

uint16_t angleToPulse(int angleDeg) {
  return map(angleDeg, 0, 180, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);

  pwm.begin();
  pwm.setOscillatorFrequency(27000000); // frequence typique du cristal interne PCA9685
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);

  Serial.println("[25_servo_pca9685] PCA9685 initialise - 4 servos en mouvement independant");
}

void loop() {
  unsigned long now = millis();
  if (now - lastStepTime >= SWEEP_INTERVAL_MS) {
    lastStepTime = now;

    for (uint8_t i = 0; i < NUM_SERVOS; i++) {
      angles[i] += directions[i];
      if (angles[i] >= 180) { angles[i] = 180; directions[i] = -1; }
      if (angles[i] <= 0)   { angles[i] = 0;   directions[i] = 1; }
      pwm.setPWM(i, 0, angleToPulse(angles[i]));
    }
  }
}
