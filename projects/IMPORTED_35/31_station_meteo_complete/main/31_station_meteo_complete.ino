/*
 * Projet 31 - Station meteo complete
 * Combine : BME280 (temp/humidite/pression) + SSD1306 (affichage) + DS3231 (RTC)
 * Cible : ESP32
 *
 * Bus I2C partage (SDA=21, SCL=22) - adresses utilisees :
 *   BME280  : 0x76
 *   SSD1306 : 0x3C
 *   DS3231  : 0x68
 * Aucun conflit d'adresse entre ces trois peripheriques (verifie).
 *
 * Architecture non bloquante : chaque sous-systeme (lecture capteur,
 * rafraichissement ecran, log serie) a son propre intervalle gere par millis(),
 * aucun delay() n'est utilise dans loop().
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

// ---------- Configuration materielle ----------
#define BME280_ADDR   0x76
#define SSD1306_ADDR  0x3C
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

Adafruit_BME280 bme;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_DS3231 rtc;

// ---------- Etat global ----------
bool bmeOk = false;
bool displayOk = false;
bool rtcOk = false;

float lastTempC = NAN;
float lastHumidity = NAN;
float lastPressureHpa = NAN;
DateTime lastDateTime;

// ---------- Cadencement non bloquant ----------
const unsigned long SENSOR_READ_INTERVAL_MS = 2000;
const unsigned long DISPLAY_UPDATE_INTERVAL_MS = 1000;
const unsigned long SERIAL_LOG_INTERVAL_MS = 5000;

unsigned long lastSensorRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastSerialLog = 0;

void readSensors() {
  if (bmeOk) {
    lastTempC = bme.readTemperature();
    lastHumidity = bme.readHumidity();
    lastPressureHpa = bme.readPressure() / 100.0F;
  }
  if (rtcOk) {
    lastDateTime = rtc.now();
  }
}

void updateDisplay() {
  if (!displayOk) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  if (rtcOk) {
    display.printf("%02d/%02d/%04d  %02d:%02d:%02d",
                    lastDateTime.day(), lastDateTime.month(), lastDateTime.year(),
                    lastDateTime.hour(), lastDateTime.minute(), lastDateTime.second());
  } else {
    display.println("RTC indisponible");
  }
  display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);

  if (bmeOk) {
    display.setTextSize(2);
    display.setCursor(0, 16);
    display.printf("%.1fC", lastTempC);

    display.setTextSize(1);
    display.setCursor(0, 38);
    display.printf("Humidite : %.1f%%", lastHumidity);
    display.setCursor(0, 50);
    display.printf("Pression : %.0f hPa", lastPressureHpa);
  } else {
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println("BME280 indisponible");
  }

  display.display();
}

void logToSerial() {
  Serial.println("========== [31_station_meteo] Releve ==========");
  if (rtcOk) {
    Serial.printf("Horodatage : %04d-%02d-%02d %02d:%02d:%02d\n",
                  lastDateTime.year(), lastDateTime.month(), lastDateTime.day(),
                  lastDateTime.hour(), lastDateTime.minute(), lastDateTime.second());
  }
  if (bmeOk) {
    Serial.printf("Temperature : %.2f C\n", lastTempC);
    Serial.printf("Humidite    : %.2f %%\n", lastHumidity);
    Serial.printf("Pression    : %.2f hPa\n", lastPressureHpa);
  }
  Serial.println("=================================================");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);
  Wire.setClock(100000);

  bmeOk = bme.begin(BME280_ADDR, &Wire);
  Serial.printf("[31_station_meteo] BME280  : %s\n", bmeOk ? "OK" : "ECHEC");
  if (bmeOk) {
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                     Adafruit_BME280::SAMPLING_X2,
                     Adafruit_BME280::SAMPLING_X16,
                     Adafruit_BME280::SAMPLING_X1,
                     Adafruit_BME280::FILTER_X16,
                     Adafruit_BME280::STANDBY_MS_500);
  }

  displayOk = display.begin(SSD1306_SWITCHCAPVCC, SSD1306_ADDR);
  Serial.printf("[31_station_meteo] SSD1306 : %s\n", displayOk ? "OK" : "ECHEC");
  if (displayOk) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Station meteo - init...");
    display.display();
  }

  rtcOk = rtc.begin(&Wire);
  Serial.printf("[31_station_meteo] DS3231  : %s\n", rtcOk ? "OK" : "ECHEC");
  if (rtcOk && rtc.lostPower()) {
    Serial.println("[31_station_meteo] RTC sans heure valide -> reglage sur date de compilation.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  readSensors();
  updateDisplay();
  Serial.println("[31_station_meteo] Station meteo prete.");
}

void loop() {
  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_READ_INTERVAL_MS) {
    lastSensorRead = now;
    readSensors();
  }

  if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
    lastDisplayUpdate = now;
    updateDisplay();
  }

  if (now - lastSerialLog >= SERIAL_LOG_INTERVAL_MS) {
    lastSerialLog = now;
    logToSerial();
  }
}
