/*
 * Projet 13 - LDR (photoresistance) - detection de luminosite
 * Cible : ESP32 / Uno
 * But : lire une luminosite relative via un pont diviseur de tension LDR + resistance fixe.
 */

#include <Arduino.h>

const uint8_t LDR_PIN = 34; // ADC1, input-only
const unsigned long SAMPLE_INTERVAL_MS = 250;
const int DARK_THRESHOLD = 1500;  // a calibrer selon votre montage / resistance fixe
const int LIGHT_THRESHOLD = 3000; // a calibrer selon votre montage / resistance fixe

unsigned long lastSampleTime = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  Serial.println("[13_ldr] Pret");
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;
    int raw = analogRead(LDR_PIN);

    String etat;
    if (raw < DARK_THRESHOLD) {
      etat = "SOMBRE";
    } else if (raw > LIGHT_THRESHOLD) {
      etat = "LUMINEUX";
    } else {
      etat = "MOYEN";
    }

    Serial.printf("[13_ldr] raw=%4d  etat=%s\n", raw, etat.c_str());
  }
}
