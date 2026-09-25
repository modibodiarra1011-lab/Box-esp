/*
 * Projet 29 - Extension GPIO MCP23017 (I2C, 16 broches supplementaires)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "Adafruit MCP23017 Arduino Library"
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>

Adafruit_MCP23X17 mcp;
const uint8_t MCP_I2C_ADDRESS = 0x20; // A0,A1,A2 a GND

const uint8_t NUM_OUTPUT_PINS = 8;  // GPA0-GPA7 utilises en sortie (chenillard)
const uint8_t INPUT_PIN = 8;        // GPB0 (broche 8) utilisee en entree avec pull-up

const unsigned long STEP_INTERVAL_MS = 150;
unsigned long lastStepTime = 0;
uint8_t currentOutputIndex = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);

  if (!mcp.begin_I2C(MCP_I2C_ADDRESS, &Wire)) {
    Serial.println("[29_mcp23017] MCP23017 non trouve ! Verifiez le cablage/adresse.");
    while (true) { delay(1000); }
  }

  for (uint8_t i = 0; i < NUM_OUTPUT_PINS; i++) {
    mcp.pinMode(i, OUTPUT);
    mcp.digitalWrite(i, LOW);
  }
  mcp.pinMode(INPUT_PIN, INPUT_PULLUP);

  Serial.println("[29_mcp23017] Extension GPIO initialisee - chenillard sur GPA0-7");
}

void loop() {
  unsigned long now = millis();
  if (now - lastStepTime >= STEP_INTERVAL_MS) {
    lastStepTime = now;

    // Eteint la sortie precedente, allume la suivante (effet chenillard)
    mcp.digitalWrite(currentOutputIndex, LOW);
    currentOutputIndex = (currentOutputIndex + 1) % NUM_OUTPUT_PINS;
    mcp.digitalWrite(currentOutputIndex, HIGH);
  }

  // Lecture de l'entree deportee (ex: bouton sur l'extenseur)
  static bool lastInputState = HIGH;
  bool inputState = mcp.digitalRead(INPUT_PIN);
  if (inputState != lastInputState) {
    lastInputState = inputState;
    Serial.printf("[29_mcp23017] Entree GPB0 = %s\n", inputState ? "HIGH" : "LOW");
  }
}
