/*
 * Projet 23 - ESP-NOW (EMETTEUR)
 * Cible : ESP32 (protocole proprietaire Espressif, faible latence, sans routeur)
 * But : envoyer periodiquement une structure de donnees a un recepteur ESP-NOW
 * identifie par son adresse MAC, sans connexion WiFi classique.
 *
 * NOTE : ce fichier est le COTE EMETTEUR. Flashez 23_espnow_receiver.ino
 * sur une deuxieme carte ESP32 pour former la paire complete.
 */

#include <esp_now.h>
#include <WiFi.h>

// Remplacez par l'adresse MAC reelle de votre recepteur
// (affichee au demarrage du sketch recepteur sur son moniteur serie)
uint8_t receiverMac[] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};

typedef struct SensorMessage {
  uint32_t counter;
  float temperature;
  unsigned long timestampMs;
} SensorMessage;

SensorMessage outgoingMessage;
esp_now_peer_info_t peerInfo;

const unsigned long SEND_INTERVAL_MS = 1000;
unsigned long lastSendTime = 0;

void onDataSent(const uint8_t *macAddr, esp_now_send_status_t status) {
  Serial.printf("[23_espnow_sender] Statut envoi : %s\n",
                status == ESP_NOW_SEND_SUCCESS ? "SUCCES" : "ECHEC");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  WiFi.mode(WIFI_STA); // ESP-NOW requiert le mode STA (sans connexion a un routeur)

  if (esp_now_init() != ESP_OK) {
    Serial.println("[23_espnow_sender] Erreur d'initialisation ESP-NOW !");
    return;
  }
  esp_now_register_send_cb(onDataSent);

  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[23_espnow_sender] Erreur lors de l'ajout du peer !");
    return;
  }

  Serial.print("[23_espnow_sender] Adresse MAC de cet emetteur : ");
  Serial.println(WiFi.macAddress());
  Serial.println("[23_espnow_sender] Pret a envoyer.");
}

void loop() {
  unsigned long now = millis();
  if (now - lastSendTime >= SEND_INTERVAL_MS) {
    lastSendTime = now;
    outgoingMessage.counter++;
    outgoingMessage.temperature = 20.0 + (float)random(-50, 50) / 10.0; // valeur simulee
    outgoingMessage.timestampMs = now;

    esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&outgoingMessage, sizeof(outgoingMessage));
    if (result == ESP_OK) {
      Serial.printf("[23_espnow_sender] Envoi #%lu (T=%.1fC)\n",
                    outgoingMessage.counter, outgoingMessage.temperature);
    } else {
      Serial.println("[23_espnow_sender] Erreur lors de l'envoi.");
    }
  }
}
