/*
 * Projet 34 - Data logger environnemental
 * Combine : DHT22 (temp/humidite, OneWire-like) + BH1750 (luminosite, I2C) + DS3231 (RTC, I2C)
 * Cible : ESP32
 *
 * Bus I2C partage (SDA=21, SCL=22) - adresses utilisees :
 *   BH1750 : 0x23
 *   DS3231 : 0x68
 * Aucun conflit d'adresse. Le DHT22 est sur un GPIO dedie hors bus I2C (GPIO4).
 *
 * Architecture non bloquante : echantillonnage periodique horodate, log
 * formate en CSV sur le port serie (pret a etre redirige vers une carte SD
 * ou un service cloud dans une evolution future du projet).
 */

#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <BH1750.h>
#include <RTClib.h>

// ---------- DHT22 ----------
#define DHT_PIN 4
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// ---------- BH1750 ----------
BH1750 lightMeter(0x23);

// ---------- DS3231 ----------
RTC_DS3231 rtc;

bool dhtOk = true;   // le DHT n'a pas de "begin()" avec retour d'erreur ; on suppose OK, verifie a la 1ere lecture
bool bhOk = false;
bool rtcOk = false;

const unsigned long SAMPLE_INTERVAL_MS = 3000; // respecte la contrainte du DHT22 (>=2s)
unsigned long lastSampleTime = 0;
uint32_t sampleId = 0;
bool csvHeaderPrinted = false;

void printCsvHeaderIfNeeded() {
  if (!csvHeaderPrinted) {
    Serial.println("id,horodatage,temperature_c,humidite_pct,luminosite_lux");
    csvHeaderPrinted = true;
  }
}

void logSample() {
  printCsvHeaderIfNeeded();
  sampleId++;

  String horodatage = "N/A";
  if (rtcOk) {
    DateTime dt = rtc.now();
    char buf[20];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second());
    horodatage = String(buf);
  }

  float tempC = dht.readTemperature();
  float humidity = dht.readHumidity();
  bool dhtValid = !(isnan(tempC) || isnan(humidity));

  float lux = bhOk ? lightMeter.readLightLevel() : -1.0;

  Serial.printf("%lu,%s,%s,%s,%s\n",
                sampleId,
                horodatage.c_str(),
                dhtValid ? String(tempC, 1).c_str() : "ERR",
                dhtValid ? String(humidity, 1).c_str() : "ERR",
                (bhOk && lux >= 0) ? String(lux, 1).c_str() : "ERR");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Wire.begin(21, 22);

  dht.begin();

  bhOk = lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  Serial.printf("[34_datalogger] BH1750 : %s\n", bhOk ? "OK" : "ECHEC");

  rtcOk = rtc.begin(&Wire);
  Serial.printf("[34_datalogger] DS3231 : %s\n", rtcOk ? "OK" : "ECHEC");
  if (rtcOk && rtc.lostPower()) {
    Serial.println("[34_datalogger] RTC sans heure valide -> reglage sur date de compilation.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("[34_datalogger] Data logger environnemental pret (format CSV).");
  lastSampleTime = millis();
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
    lastSampleTime = now;
    logSample();
  }
}
