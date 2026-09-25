/*
 * Projet 33 - Robot d'evitement d'obstacles
 * Combine : HC-SR04 (distance) + 2x Servos SG90 (direction "tete" + "roue" de test)
 * Cible : ESP32
 *
 * NOTE PEDAGOGIQUE : ce template pilote deux servos pour simuler la logique
 * de navigation (un servo "tete" pour orienter le capteur ultrason en balayage,
 * un servo "direction" pour representer l'angle de braquage). Pour un vrai
 * robot a roues, remplacez le servo de direction par un driver moteur (ex: L298N)
 * pilote en PWM sur les memes principes non bloquants.
 *
 * Machine a etats non bloquante : SCAN -> DECISION -> AVANCE -> EVITEMENT -> SCAN...
 */

#include <Arduino.h>
#include <ESP32Servo.h>

// ---------- Capteur ultrason (voir wiring.md : diviseur de tension obligatoire sur ECHO) ----------
const uint8_t TRIG_PIN = 5;
const uint8_t ECHO_PIN = 18;
const unsigned long ECHO_TIMEOUT_US = 30000UL;
const float OBSTACLE_THRESHOLD_CM = 20.0;

// ---------- Servos ----------
Servo headServo;      // oriente le capteur ultrason (balayage gauche/droite)
Servo steeringServo;  // represente la direction du robot (a remplacer par un driver moteur reel)
const uint8_t HEAD_SERVO_PIN = 16;
const uint8_t STEERING_SERVO_PIN = 17;

const int HEAD_CENTER = 90;
const int HEAD_LEFT = 150;
const int HEAD_RIGHT = 30;

// ---------- Machine a etats ----------
enum RobotState { SCAN_CENTER, SCAN_LEFT, SCAN_RIGHT, DECISION, AVOIDING };
RobotState state = SCAN_CENTER;
unsigned long stateChangeTime = 0;
const unsigned long SERVO_SETTLE_MS = 300; // temps d'attente pour que le servo atteigne sa position

float distCenter = -1, distLeft = -1, distRight = -1;

float measureDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1.0;
  return (duration * 0.0343) / 2.0;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  headServo.setPeriodHertz(50);
  headServo.attach(HEAD_SERVO_PIN, 500, 2400);
  steeringServo.setPeriodHertz(50);
  steeringServo.attach(STEERING_SERVO_PIN, 500, 2400);

  headServo.write(HEAD_CENTER);
  steeringServo.write(90); // position "tout droit"

  state = SCAN_CENTER;
  stateChangeTime = millis();

  Serial.println("[33_robot] Robot d'evitement pret - demarrage du scan.");
}

void loop() {
  unsigned long now = millis();

  switch (state) {
    case SCAN_CENTER:
      headServo.write(HEAD_CENTER);
      if (now - stateChangeTime >= SERVO_SETTLE_MS) {
        distCenter = measureDistanceCm();
        Serial.printf("[33_robot] Distance centre = %.1f cm\n", distCenter);
        state = SCAN_LEFT;
        stateChangeTime = now;
      }
      break;

    case SCAN_LEFT:
      headServo.write(HEAD_LEFT);
      if (now - stateChangeTime >= SERVO_SETTLE_MS) {
        distLeft = measureDistanceCm();
        Serial.printf("[33_robot] Distance gauche = %.1f cm\n", distLeft);
        state = SCAN_RIGHT;
        stateChangeTime = now;
      }
      break;

    case SCAN_RIGHT:
      headServo.write(HEAD_RIGHT);
      if (now - stateChangeTime >= SERVO_SETTLE_MS) {
        distRight = measureDistanceCm();
        Serial.printf("[33_robot] Distance droite = %.1f cm\n", distRight);
        state = DECISION;
        stateChangeTime = now;
      }
      break;

    case DECISION:
      if (distCenter > OBSTACLE_THRESHOLD_CM || distCenter < 0) {
        Serial.println("[33_robot] Voie libre -> avance tout droit.");
        steeringServo.write(90);
      } else if (distLeft > distRight) {
        Serial.println("[33_robot] Obstacle detecte -> braquage a gauche.");
        steeringServo.write(150);
      } else {
        Serial.println("[33_robot] Obstacle detecte -> braquage a droite.");
        steeringServo.write(30);
      }
      state = AVOIDING;
      stateChangeTime = now;
      break;

    case AVOIDING:
      // Laisse le temps a la "manoeuvre" avant de reprendre un cycle de scan complet
      if (now - stateChangeTime >= 500) {
        state = SCAN_CENTER;
        stateChangeTime = now;
      }
      break;
  }
}
