/*
 * Projet 23 - ESP-NOW (RECEPTEUR)
 * Cible : ESP32
 * But : recevoir les messages envoyes par 23_espnow_sender.ino et les afficher.
 *
 * ETAPE PREALABLE : flashez d'abord ce sketch sur la carte "recepteur",
 * relevez son adresse MAC affichee au demarrage, puis reportez-la dans
 * le tableau `receiverMac[]` du sketch 23_espnow_sender.ino avant de le flasher
 * sur la seconde carte.
 */

#include <esp_now.h>
#include <WiFi.h>

typedef struct SensorMessage {
  uint32_t counter;
  float temperature;
  unsigned long timestampMs;
} SensorMessage;

SensorMessage incomingMessage;

void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  memcpy(&incomingMessage, data, sizeof(incomingMessage));
  Serial.printf("[23_espnow_receiver] Recu #%lu  T=%.1fC  t_emetteur=%lums\n",
                incomingMessage.counter,
                incomingMessage.temperature,
                incomingMessage.timestampMs);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  WiFi.mode(WIFI_STA);

  Serial.print("[23_espnow_receiver] Adresse MAC de ce recepteur (a reporter cote emetteur) : ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[23_espnow_receiver] Erreur d'initialisation ESP-NOW !");
    return;
  }
  esp_now_register_recv_cb(onDataReceived);
  Serial.println("[23_espnow_receiver] Pret a recevoir.");
}

void loop() {
  // Tout est gere en arriere-plan via la callback onDataReceived (non bloquant).
}
