/*
 * Projet 28 - Ecran TFT ILI9341 (320x240, SPI)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "Adafruit ILI9341" + "Adafruit GFX Library"
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
// SCK=GPIO18, MOSI=GPIO23, MISO=GPIO19 (bus VSPI materiel par defaut)

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

const unsigned long UPDATE_INTERVAL_MS = 1000;
unsigned long lastUpdateTime = 0;
uint32_t frameCounter = 0;

void drawStaticUI() {
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("ESP32 LAB");
  tft.setTextSize(1);
  tft.setCursor(10, 35);
  tft.println("Ecran ILI9341 320x240 SPI");
  tft.drawFastHLine(0, 50, tft.width(), ILI9341_CYAN);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  tft.begin();
  tft.setRotation(1); // paysage
  drawStaticUI();

  Serial.println("[28_ili9341] Ecran TFT initialise.");
}

void loop() {
  unsigned long now = millis();
  if (now - lastUpdateTime >= UPDATE_INTERVAL_MS) {
    lastUpdateTime = now;
    frameCounter++;

    // Efface uniquement la zone dynamique (evite un fillScreen complet a chaque frame)
    tft.fillRect(10, 70, 300, 60, ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_GREEN);
    tft.setCursor(10, 70);
    tft.printf("Uptime: %lus", now / 1000);
    tft.setCursor(10, 95);
    tft.printf("Frame : %lu", frameCounter);
  }
}
