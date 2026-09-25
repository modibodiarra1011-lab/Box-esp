/*
 * Projet 17 - Lecteur RFID RC522 (SPI)
 * Cible : ESP32 / Uno
 * Bibliotheque requise : "MFRC522" (par miguelbalboa)
 */

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#define RFID_SS_PIN  5
#define RFID_RST_PIN 27

MFRC522 mfrc522(RFID_SS_PIN, RFID_RST_PIN);

void printUid(MFRC522::Uid uid) {
  Serial.print("[17_rc522] UID détectée : ");
  for (byte i = 0; i < uid.size; i++) {
    Serial.printf("%02X", uid.uidByte[i]);
    if (i < uid.size - 1) Serial.print(":");
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  SPI.begin(18, 19, 23, RFID_SS_PIN); // SCK, MISO, MOSI, SS (mapping explicite ESP32)
  mfrc522.PCD_Init();
  delay(50);

  byte version = mfrc522.PCD_ReadRegister(MFRC522::VersionReg);
  Serial.printf("[17_rc522] Version du firmware MFRC522 : 0x%02X\n", version);
  if (version == 0x00 || version == 0xFF) {
    Serial.println("[17_rc522] ATTENTION : module non detecte, verifiez le cablage SPI.");
  }
  Serial.println("[17_rc522] Approchez un badge/carte RFID...");
}

void loop() {
  // Recherche une nouvelle carte
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  printUid(mfrc522.uid);

  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.printf("[17_rc522] Type de carte : %s\n", mfrc522.PICC_GetTypeName(piccType));

  mfrc522.PICC_HaltA();      // arrete la communication avec la carte courante
  mfrc522.PCD_StopCrypto1(); // libere le chiffrement (si carte protegee)
}
