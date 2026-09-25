/*
 * Projet 20 - Boucle UART (Serial Loopback) sur UART2
 * Cible : ESP32 (3 UART materiels disponibles) / Uno (SoftwareSerial necessaire)
 * But : valider une liaison serie materielle en reliant physiquement TX a RX,
 * et verifier l'integrite des donnees envoyees/recues.
 */

#include <Arduino.h>

// UART2 materiel de l'ESP32 (distinct de Serial=UART0 utilise pour le debug USB)
HardwareSerial LoopSerial(2);

const uint8_t UART2_TX_PIN = 17;
const uint8_t UART2_RX_PIN = 16;
const unsigned long SEND_INTERVAL_MS = 1000;
unsigned long lastSendTime = 0;
uint32_t messageCounter = 0;

void setup() {
  Serial.begin(115200);       // console de debug (USB)
  LoopSerial.begin(9600, SERIAL_8N1, UART2_RX_PIN, UART2_TX_PIN);
  delay(200);
  Serial.println("[20_serial_loopback] Pret - TX17 doit etre relie physiquement a RX16");
}

void loop() {
  unsigned long now = millis();

  if (now - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = now;
    messageCounter++;
    String message = "PING#" + String(messageCounter);
    LoopSerial.println(message);
    Serial.printf("[20_serial_loopback] Envoye : %s\n", message.c_str());
  }

  // Lecture non bloquante de ce qui revient par la boucle physique
  while (LoopSerial.available() > 0) {
    String received = LoopSerial.readStringUntil('\n');
    received.trim();
    if (received.length() > 0) {
      Serial.printf("[20_serial_loopback] Recu  : %s (boucle OK)\n", received.c_str());
    }
  }
}
