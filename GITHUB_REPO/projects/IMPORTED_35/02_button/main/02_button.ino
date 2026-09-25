/*
 * Projet 02 - Bouton poussoir avec anti-rebond logiciel
 * Cible : ESP32 / Uno
 * But : lire un bouton poussoir de façon fiable (debounce par millis()),
 * et déclencher une action (toggle LED) sur front descendant.
 */

#include <Arduino.h>

const uint8_t BUTTON_PIN = 4;      // Entrée bouton (pull-up interne activée)
const uint8_t LED_PIN = LED_BUILTIN;
const unsigned long DEBOUNCE_DELAY_MS = 50;

bool lastRawState = HIGH;
bool stableState = HIGH;
unsigned long lastEdgeTime = 0;
bool ledState = LOW;

void setup() {
  Serial.begin(115200);
  delay(200);
  // INPUT_PULLUP : bouton câblé entre la broche et GND -> lecture HIGH au repos, LOW appuyé
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, ledState);
  Serial.println("[02_button] Pret - appuyez sur le bouton pour basculer la LED");
}

void loop() {
  bool raw = digitalRead(BUTTON_PIN);

  if (raw != lastRawState) {
    lastEdgeTime = millis(); // redemarre le temporisateur anti-rebond
    lastRawState = raw;
  }

  if ((millis() - lastEdgeTime) > DEBOUNCE_DELAY_MS) {
    if (raw != stableState) {
      stableState = raw;
      if (stableState == LOW) { // front descendant = appui valide
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
        Serial.printf("[02_button] Appui detecte -> LED %s\n", ledState ? "ON" : "OFF");
      }
    }
  }
}
