/*
 * Projet 26 - Module GPS NEO-6M (UART)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "TinyGPS++" (par Mikal Hart)
 */

#include <Arduino.h>
#include <TinyGPSPlus.h>

TinyGPSPlus gps;
HardwareSerial gpsSerial(2); // UART2 materiel

const uint8_t GPS_RX_PIN = 16; // relie au TX du module GPS
const uint8_t GPS_TX_PIN = 17; // relie au RX du module GPS
const unsigned long REPORT_INTERVAL_MS = 1000;
unsigned long lastReportTime = 0;

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN); // 9600 bauds = defaut NEO-6M
  delay(200);
  Serial.println("[26_gps_neo6m] Pret - en attente de fix satellite (peut prendre 1-2 min en exterieur)");
}

void loop() {
  // Alimente le parseur en continu avec les octets bruts recus du GPS (non bloquant)
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  unsigned long now = millis();
  if (now - lastReportTime >= REPORT_INTERVAL_MS) {
    lastReportTime = now;

    if (gps.location.isValid()) {
      Serial.printf("[26_gps_neo6m] Lat=%.6f Lon=%.6f Alt=%.1fm Satellites=%d\n",
                    gps.location.lat(), gps.location.lng(),
                    gps.altitude.meters(), gps.satellites.value());
    } else {
      Serial.println("[26_gps_neo6m] En attente de fix GPS (aucune position valide pour l'instant)...");
    }

    if (gps.date.isValid() && gps.time.isValid()) {
      Serial.printf("[26_gps_neo6m] Date/heure UTC : %02d/%02d/%04d %02d:%02d:%02d\n",
                    gps.date.day(), gps.date.month(), gps.date.year(),
                    gps.time.hour(), gps.time.minute(), gps.time.second());
    }

    // Diagnostic : si aucun caractere n'a jamais ete recu, verifier le cablage
    if (gps.charsProcessed() < 10) {
      Serial.println("[26_gps_neo6m] ATTENTION : aucune donnee recue, verifiez le cablage RX/TX.");
    }
  }
}
