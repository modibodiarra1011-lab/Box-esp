/*
 * Projet 03 - Potentiomètre (lecture analogique)
 * Cible : ESP32 (ADC 12 bits, 0-4095) / Uno (ADC 10 bits, 0-1023)
 * But : lire une tension analogique et la convertir en volts + pourcentage.
 */

#include <Arduino.h>

const uint8_t POT_PIN = 34;           // GPIO34 = entrée ADC1, "input only" sur ESP32
const float ADC_MAX = 4095.0;         // 12 bits sur ESP32 (utiliser 1023.0 sur Uno)
const float V_REF = 3.3;              // Tension de référence ESP32 (5.0 sur Uno)
const unsigned long SAMPLE_INTERVAL_MS = 200;

unsigned long lastSampleTime = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  analogReadResolution(12);   // Explicite sur ESP32 (par defaut deja 12 bits)
  analogSetAttenuation(ADC_11db); // Permet de mesurer jusqu'a ~3.3V (attenuation ~11dB)
  Serial.println("[03_potentiometer] Pret");
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;
    int raw = analogRead(POT_PIN);
    float voltage = (raw / ADC_MAX) * V_REF;
    float percent = (raw / ADC_MAX) * 100.0;
    Serial.printf("[03_potentiometer] raw=%4d  V=%.3fV  %.1f%%\n", raw, voltage, percent);
  }
}
