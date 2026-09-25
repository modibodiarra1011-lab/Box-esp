/*
 * Projet 01 - Blink LED
 * Cible principale : ESP32 DevKit (compatible Arduino Uno avec adaptation de broche)
 *
 * But : faire clignoter une LED sans bloquer le CPU (utilisation de millis()
 * au lieu de delay()), afin que ce squelette soit réutilisable dans des
 * projets multitâches (voir dossier 24_worker_diagnostic et les suites).
 *
 * Câblage : voir wiring.md
 */

#include <Arduino.h>

// Sur la plupart des cartes ESP32 DevKit, LED_BUILTIN correspond à GPIO2.
// Si votre carte n'a pas de LED_BUILTIN définie par le core, décommentez la ligne suivante :
// #define LED_BUILTIN 2

const uint8_t LED_PIN = LED_BUILTIN;
const unsigned long BLINK_INTERVAL_MS = 500;

unsigned long lastToggleTime = 0;
bool ledState = LOW;

void setup() {
  Serial.begin(115200);
  delay(200); // laisse le temps au moniteur série de s'attacher
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);
  Serial.println("[01_blink] Demarrage - clignotement non bloquant (millis)");
}

void loop() {
  unsigned long now = millis();
  if (now - lastToggleTime >= BLINK_INTERVAL_MS) {
    lastToggleTime = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    Serial.printf("[01_blink] LED = %s (t=%lu ms)\n", ledState ? "ON" : "OFF", now);
  }
  // Le reste de loop() reste disponible pour d'autres taches non bloquantes.
}
