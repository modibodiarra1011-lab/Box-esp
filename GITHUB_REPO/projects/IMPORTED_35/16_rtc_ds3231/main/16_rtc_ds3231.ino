/*
 * Projet 16 - Horloge temps reel DS3231 (RTC, I2C)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "RTClib" (Adafruit)
 */

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;
const unsigned long READ_INTERVAL_MS = 1000;
unsigned long lastReadTime = 0;
bool rtcFound = false;

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);

  if (!rtc.begin(&Wire)) {
    Serial.println("[16_rtc_ds3231] DS3231 non trouve ! Verifiez le cablage.");
    return;
  }
  rtcFound = true;

  if (rtc.lostPower()) {
    Serial.println("[16_rtc_ds3231] RTC sans alimentation de secours ou premiere mise en route.");
    Serial.println("[16_rtc_ds3231] Reglage de l'heure sur la date/heure de compilation.");
    // Regle l'horloge sur la date/heure de compilation du sketch (a ajuster manuellement si besoin)
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("[16_rtc_ds3231] RTC pret.");
}

void loop() {
  if (!rtcFound) return;
  unsigned long now = millis();
  if (now - lastReadTime >= READ_INTERVAL_MS) {
    lastReadTime = now;
    DateTime dt = rtc.now();
    float tempC = rtc.getTemperature(); // capteur de temperature interne au DS3231

    Serial.printf("[16_rtc_ds3231] %04d-%02d-%02d %02d:%02d:%02d  T_interne=%.1fC\n",
                  dt.year(), dt.month(), dt.day(),
                  dt.hour(), dt.minute(), dt.second(),
                  tempC);
  }
}
