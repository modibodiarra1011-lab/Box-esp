/*
 * Projet 10 - Capteur ultrason HC-SR04
 * Cible : ESP32 / Uno
 * But : mesurer une distance par temps de vol ultrason (pulseIn), sans delay()
 * bloquant pour le declenchement (utilisation de millis() pour cadencer les mesures).
 *
 * ATTENTION MATERIELLE : le HC-SR04 est un capteur 5V. Voir wiring.md pour le
 * diviseur de tension obligatoire sur ECHO cote ESP32.
 */

#include <Arduino.h>

const uint8_t TRIG_PIN = 5;
const uint8_t ECHO_PIN = 18; // relie via diviseur de tension (voir wiring.md)
const unsigned long MEASURE_INTERVAL_MS = 300;
const unsigned long ECHO_TIMEOUT_US = 30000UL; // ~5m max, evite un blocage si pas d'echo

unsigned long lastMeasureTime = 0;

float measureDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) {
    return -1.0; // pas d'echo recu (hors portee ou obstacle absent)
  }
  // vitesse du son ~343 m/s a 20C => 0.0343 cm/us, aller-retour => /2
  return (duration * 0.0343) / 2.0;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  Serial.println("[10_hcsr04] Pret");
}

void loop() {
  unsigned long now = millis();
  if (now - lastMeasureTime >= MEASURE_INTERVAL_MS) {
    lastMeasureTime = now;
    float distance = measureDistanceCm();
    if (distance < 0) {
      Serial.println("[10_hcsr04] Hors de portee / pas d'echo");
    } else {
      Serial.printf("[10_hcsr04] Distance = %.1f cm\n", distance);
    }
  }
}
