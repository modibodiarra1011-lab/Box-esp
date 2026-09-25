/*
 * Projet 09 - Ecran OLED SSD1306 (128x64, I2C)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "Adafruit SSD1306" + "Adafruit GFX Library"
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SSD1306_I2C_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const unsigned long UPDATE_INTERVAL_MS = 1000;
unsigned long lastUpdate = 0;
uint32_t counter = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS)) {
    Serial.println("[09_ssd1306] Echec d'initialisation de l'ecran !");
    while (true) { delay(1000); } // arret volontaire : materiel indisponible
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 LAB - SSD1306");
  display.display();
  Serial.println("[09_ssd1306] Ecran initialise.");
}

void loop() {
  unsigned long now = millis();
  if (now - lastUpdate >= UPDATE_INTERVAL_MS) {
    lastUpdate = now;
    counter++;

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("ESP32 LAB - SSD1306");
    display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);

    display.setTextSize(2);
    display.setCursor(0, 20);
    display.printf("Uptime:%lus", now / 1000);

    display.setTextSize(1);
    display.setCursor(0, 45);
    display.printf("Cycles: %lu", counter);
    display.display();
  }
}
