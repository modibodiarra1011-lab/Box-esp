/*
 * Projet 21 - Boucle SPI (SPI Loopback)
 * Cible : ESP32 / Uno
 * But : valider le bus SPI materiel en reliant physiquement MOSI a MISO,
 * et verifier que chaque octet envoye est bien recu identique (mode maitre seul).
 */

#include <Arduino.h>
#include <SPI.h>

const uint8_t SPI_SCK_PIN  = 18;
const uint8_t SPI_MISO_PIN = 19;
const uint8_t SPI_MOSI_PIN = 23;
const uint8_t SPI_CS_PIN   = 5; // non utilise electriquement ici (pas d'esclave), mais requis par certaines libs

const unsigned long TEST_INTERVAL_MS = 1000;
unsigned long lastTestTime = 0;
uint8_t testByte = 0x00;

SPIClass spiBus(VSPI);

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(SPI_CS_PIN, OUTPUT);
  digitalWrite(SPI_CS_PIN, HIGH);

  spiBus.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, SPI_CS_PIN);
  Serial.println("[21_spi_loopback] Pret - MOSI(23) doit etre relie physiquement a MISO(19)");
}

void loop() {
  unsigned long now = millis();
  if (now - lastTestTime >= TEST_INTERVAL_MS) {
    lastTestTime = now;
    testByte++;

    spiBus.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(SPI_CS_PIN, LOW);
    uint8_t received = spiBus.transfer(testByte);
    digitalWrite(SPI_CS_PIN, HIGH);
    spiBus.endTransaction();

    if (received == testByte) {
      Serial.printf("[21_spi_loopback] OK  : envoye=0x%02X recu=0x%02X\n", testByte, received);
    } else {
      Serial.printf("[21_spi_loopback] ECHEC : envoye=0x%02X recu=0x%02X (verifiez le jumper MOSI-MISO)\n",
                    testByte, received);
    }
  }
}
