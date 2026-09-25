/*
 * Projet 18 - Bandeau LED adressable WS2812 (NeoPixel)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "Adafruit NeoPixel"
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 5
#define LED_COUNT 8

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

const unsigned long ANIMATION_INTERVAL_MS = 60;
unsigned long lastStepTime = 0;
uint16_t animationStep = 0;

// Roue de couleurs classique (0-255) -> code couleur RGB
uint32_t wheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return strip.Color(255 - pos * 3, 0, pos * 3);
  } else if (pos < 170) {
    pos -= 85;
    return strip.Color(0, pos * 3, 255 - pos * 3);
  } else {
    pos -= 170;
    return strip.Color(pos * 3, 255 - pos * 3, 0);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  strip.begin();
  strip.setBrightness(60); // limite la luminosite pour reduire la consommation
  strip.show();            // eteint toutes les LED au demarrage
  Serial.println("[18_ws2812] Pret - animation arc-en-ciel non bloquante");
}

void loop() {
  unsigned long now = millis();
  if (now - lastStepTime >= ANIMATION_INTERVAL_MS) {
    lastStepTime = now;
    animationStep = (animationStep + 1) % 256;

    for (int i = 0; i < LED_COUNT; i++) {
      byte colorIndex = (i * 256 / LED_COUNT + animationStep) & 255;
      strip.setPixelColor(i, wheel(colorIndex));
    }
    strip.show();
  }
}
