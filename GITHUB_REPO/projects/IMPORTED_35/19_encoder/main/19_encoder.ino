/*
 * Projet 19 - Encodeur rotatif incrémental (type KY-040)
 * Cible : ESP32 / Uno
 * But : compter les crans de rotation via interruptions materielles,
 * et lire le bouton poussoir integre a l'encodeur.
 */

#include <Arduino.h>

const uint8_t ENCODER_CLK_PIN = 32;
const uint8_t ENCODER_DT_PIN  = 33;
const uint8_t ENCODER_SW_PIN  = 25;

volatile long encoderPosition = 0;
volatile bool lastClkState = HIGH;

const unsigned long BUTTON_DEBOUNCE_MS = 50;
unsigned long lastButtonEdgeTime = 0;
bool lastButtonRaw = HIGH;
bool lastButtonStable = HIGH;

// Routine d'interruption : appelee a chaque changement d'etat de CLK
void IRAM_ATTR onEncoderChange() {
  bool clkState = digitalRead(ENCODER_CLK_PIN);
  if (clkState != lastClkState) {
    bool dtState = digitalRead(ENCODER_DT_PIN);
    if (dtState != clkState) {
      encoderPosition++;
    } else {
      encoderPosition--;
    }
  }
  lastClkState = clkState;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(ENCODER_CLK_PIN, INPUT_PULLUP);
  pinMode(ENCODER_DT_PIN, INPUT_PULLUP);
  pinMode(ENCODER_SW_PIN, INPUT_PULLUP);

  lastClkState = digitalRead(ENCODER_CLK_PIN);
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK_PIN), onEncoderChange, CHANGE);

  Serial.println("[19_encoder] Pret - tournez l'encodeur ou appuyez sur son bouton");
}

void loop() {
  static long lastReportedPosition = 0;
  long currentPosition = encoderPosition; // copie atomique locale

  if (currentPosition != lastReportedPosition) {
    lastReportedPosition = currentPosition;
    Serial.printf("[19_encoder] Position = %ld\n", currentPosition);
  }

  // Anti-rebond logiciel du bouton poussoir de l'encodeur
  bool raw = digitalRead(ENCODER_SW_PIN);
  if (raw != lastButtonRaw) {
    lastButtonEdgeTime = millis();
    lastButtonRaw = raw;
  }
  if ((millis() - lastButtonEdgeTime) > BUTTON_DEBOUNCE_MS) {
    if (raw != lastButtonStable) {
      lastButtonStable = raw;
      if (lastButtonStable == LOW) {
        Serial.println("[19_encoder] Bouton presse -> reset position a 0");
        encoderPosition = 0;
      }
    }
  }
}
